#import "AppDelegate.h"

#include <cinttypes>

#import <AudioToolbox/AudioToolbox.h>

#include "synth/voice_assigner.hpp"

using SampleType = Float32;  // TODO: use fixed point.
static constexpr UInt32 bufferSize = 128 * sizeof (SampleType);
static constexpr auto numBuffers = 8U;
static constexpr NSTimeInterval lockWaitTime = 1.0;


// audio description
// ~~~~~~~~~~~~~~~~~
static AudioStreamBasicDescription audioDescription (void) {
  AudioStreamBasicDescription d = {0};
  d.mSampleRate = static_cast<Float64> (synth::oscillator::sample_rate);
  d.mFormatID = kAudioFormatLinearPCM;
  d.mFormatFlags = kLinearPCMFormatFlagIsFloat;
  d.mBytesPerPacket = UInt32{sizeof (SampleType)};
  d.mFramesPerPacket = UInt32{1};
  d.mBytesPerFrame = d.mBytesPerPacket * d.mFramesPerPacket;
  d.mChannelsPerFrame = UInt32{1};
  d.mBitsPerChannel = UInt32{8 * sizeof (SampleType)};
  return d;
}

static void stopAudio (AudioQueueRef queue) {
  if (OSStatus const erc = ::AudioQueueFlush (queue)) {
    NSLog (@"AudioQueueFlush returned %d", static_cast<int> (erc));
  }
  if (OSStatus const erc = ::AudioQueueStop (queue, true /*immediate*/)) {
    NSLog (@"AudioQueueStop returned %d", static_cast<int> (erc));
  }
  if (OSStatus const erc = ::AudioQueueDispose (queue, true /*immediate*/)) {
    NSLog (@"AudioQueueDispose returned %d", static_cast<int> (erc));
  }
}

@interface AppDelegate () {
  Boolean running_;
  AudioQueueRef queue_;
  AudioQueueBufferRef *buffers_;
  NSLock *lock_;

  std::unique_ptr<synth::voice_assigner> voices_;
  MIDIPortRef port_;
  MIDIEndpointRef source_;
}
@end

@implementation AppDelegate

// init
// ~~~~
- (id)init {
  self = [super init];
  if (self) {
    running_ = NO;
    queue_ = nil;
    buffers_ = nil;
    lock_ = [NSLock new];
    voices_.reset (new synth::voice_assigner);
    //    osc_->set_frequency (synth::oscillator::frequency::fromfp (440.0));
    port_ = 0;
    source_ = 0;
  }
  return self;
}

// show error
// ~~~~~~~~~~
- (void)showError:(OSStatus)erc {
#if TARGET_OS_OSX
  [[NSAlert alertWithError:[NSError errorWithDomain:NSOSStatusErrorDomain code:erc
                                           userInfo:nil]] runModal];
#elif TARGET_OS_IOS
#if 0
  UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"My Alert"
                                 message:@"This is an alert."
                                 preferredStyle:UIAlertControllerStyleAlert];

  UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
     handler:^(UIAlertAction * action) {}];

  [alert addAction:defaultAction];
  [self presentViewController:alert animated:YES completion:nil];
#endif
#endif
}

// enqueue audio buffer
// ~~~~~~~~~~~~~~~~~~~~
- (OSStatus)enqueueAudioBuffer:(AudioQueueBufferRef)buffer {
  UInt32 samples = buffer->mAudioDataBytesCapacity / sizeof (SampleType);
  auto *const first = static_cast<SampleType *> (buffer->mAudioData);
  auto *const last = first + samples;

  double masterVolume = 0.5;

  OSStatus erc = noErr;
  BOOL running = NO;
  if ([lock_ lockBeforeDate:[NSDate dateWithTimeIntervalSinceNow:lockWaitTime]]) {
    running = running_;
    if (running) {
      for (auto *it = first; it != last; ++it) {
        *it = voices_->tick () * masterVolume;
      }
    }
    [lock_ unlock];
    buffer->mAudioDataByteSize = (last - first) * sizeof (SampleType);
  } else {
    NSLog (@"enqueueAudioBuffer could not obtain the lock in a reasonable time!");
  }

  if (running) {
    erc = ::AudioQueueEnqueueBuffer (queue_, buffer, 0U, nullptr);
    if (erc != noErr) {
      NSLog (@"AudioQueueEnqueueBuffer error %d", erc);
    }
  }
  return erc;
}

// read MIDI packet list
// ~~~~~~~~~~~~~~~~~~~~~
- (void)readMIDIPacketList:(MIDIPacketList const *)pktlist {
  auto const *packet = pktlist->packet;
  for (UInt32 packet_ctr = 0; packet_ctr < pktlist->numPackets; ++packet_ctr) {
    Byte const *byte = packet->data;
    Byte const *const end = byte + packet->length;
    while (byte != end) {
      uint8_t const message = *(byte++);
      // Status bytes are eight-bit binary numbers in which the Most Significant Bit (MSB) is set
      // (binary 1). Status bytes serve to identify the message type, that is, the purpose of the
      // Data bytes which follow it.
      unsigned const chan = message & 0x0F;
      switch (message >> 4) {
        case 0b1000:  // Note-Off
        {
          unsigned const note = *(byte++);      // TODO: bit 7 must be 0.
          unsigned const velocity = *(byte++);  // TODO: bit 7 must be 0.
          NSLog (@"Note off chan=%u, note=%u, velocity=%u", chan, note, velocity);
          if ([self->lock_ lockBeforeDate:[NSDate dateWithTimeIntervalSinceNow:lockWaitTime]]) {
            self->voices_->note_off (note);
            [self->lock_ unlock];
          }
        } break;
        case 0b1001:  // Note-On
        {
          unsigned const note = *(byte++);      // TODO: bit 7 must be 0.
          unsigned const velocity = *(byte++);  // TODO: bit 7 must be 0.
          if ([self->lock_ lockBeforeDate:[NSDate dateWithTimeIntervalSinceNow:lockWaitTime]]) {
            if (velocity == 0) {
              NSLog (@"Note off chan=%u, note=%u, velocity=%u", chan, note, velocity);
              self->voices_->note_off (note);
            } else {
              NSLog (@"Note on chan=%u, note=%u, velocity=%u", chan, note, velocity);
              self->voices_->note_on (note);
            }
            [self->lock_ unlock];
          }
        } break;
        case 0b1010:  // Polyphonic Key Pressure
        {
          unsigned const note = *(byte++);      // TODO: bit 7 must be 0.
          unsigned const velocity = *(byte++);  // TODO: bit 7 must be 0.
          NSLog (@"Poly Pressure chan=%u, note=%u, velocity=%u", chan, note, velocity);
        } break;
        case 0b1011:  // Control Change
        {
          unsigned const controller = *(byte++);  // TODO: bit 7 must be 0.
          unsigned const value = *(byte++);       // TODO: bit 7 must be 0.
          NSLog (@"CC chan=%u, controller=%u, value=%u", chan, controller, value);
        } break;
        case 0b1100:  // Program change
        {
          unsigned const program_number = *(byte++);  // TODO: bit 7 must be 0.
          NSLog (@"Program change chan=%u, no.=%u", chan, program_number);
        } break;
        case 0b1101:  // Channel Pressure (aftertouch)
        {
          unsigned const program_number = *(byte++);  // TODO: bit 7 must be 0.
          NSLog (@"Channel Pressure chan=%u, value=%u", chan, program_number);
        } break;
        case 0b1110:  // Pitch Bend
        {
          uint16_t const low = *(byte++);   // TODO: bit 7 must be 0.
          uint16_t const high = *(byte++);  // TODO: bit 7 must be 0.
          NSLog (@"Pitch Bend chan=%u, value=%u", chan, (high << 7) | low);
        } break;
        case 0xF0:  // System Message: skip args
        default:
          break;
      }
    }
    packet = MIDIPacketNext (packet);
  }
}

// read MIDI
// ~~~~~~~~~
static void readMIDI (MIDIPacketList const *const packetList, void *userData, void *connRefCon) {
  if (AppDelegate *const ad = (__bridge AppDelegate *)connRefCon) {
    [ad readMIDIPacketList:packetList];
  }
}

static NSString *midiTypeName (MIDIObjectType type) {
  switch (type) {
    case kMIDIObjectType_Other:
      return @"kMIDIObjectType_Other";
    case kMIDIObjectType_Device:
      return @"kMIDIObjectType_Device";
    case kMIDIObjectType_Entity:
      return @"kMIDIObjectType_Entity";
    case kMIDIObjectType_Source:
      return @"kMIDIObjectType_Source";
    case kMIDIObjectType_Destination:
      return @"kMIDIObjectType_Destination";

    case kMIDIObjectType_ExternalDevice:
      return @"kMIDIObjectType_ExternalDevice";
    case kMIDIObjectType_ExternalEntity:
      return @"kMIDIObjectType_ExternalEntity";
    case kMIDIObjectType_ExternalSource:
      return @"kMIDIObjectType_ExternalSource";
    case kMIDIObjectType_ExternalDestination:
      return @"kMIDIObjectType_ExternalDestination";
  }
  return @"unknown";
}

- (OSStatus)disconnectFromMIDISource {
  OSStatus erc = ::MIDIPortDisconnectSource (self->port_, self->source_);
  if (erc != noErr) {
    return erc;
  }
  self->source_ = 0;
  return noErr;
}

- (OSStatus)connectToMIDIUniqueID:(MIDIUniqueID)uniqueID {
  OSStatus erc = 0;

  MIDIEndpointRef endpoint = 0;
  MIDIObjectType objectType = kMIDIObjectType_Other;
  erc = ::MIDIObjectFindByUniqueID (uniqueID, &endpoint, &objectType);
  if (erc != noErr) {
    NSLog (@"reconnectToMIDIUniqueID.MIDIObjectFindByUniqueID: returned %d", erc);
    return erc;
  }
  if (objectType != kMIDIObjectType_Source && objectType != kMIDIObjectType_ExternalSource) {
    NSLog (@"reconnectToMIDIUniqueID wrong endpoint type");
    return kMIDIWrongEndpointType;
  }

  if (self->source_ != 0) {
    // Cleanly disconnect from our existing source if there is one.
    erc = ::MIDIPortDisconnectSource (self->port_, self->source_);
    self->source_ = 0;
  }

  CFStringRef sourceName = Nil;
  erc = MIDIObjectGetStringProperty (endpoint, kMIDIPropertyName, &sourceName);
  if (erc != noErr) {
    NSLog (@"connectToMIDIUniqueID (child kMIDIPropertyName) failed %d", erc);
  } else {
    NSLog (@"Connecting to MIDI source '%@'", sourceName);
    CFRelease (sourceName);
    sourceName = Nil;
  }

  self->source_ = endpoint;
  return ::MIDIPortConnectSource (self->port_, endpoint, (__bridge void *)self /*connRefCon*/);
}

- (OSStatus)midiObjectRemovedNotificaton:(MIDIObjectAddRemoveNotification const *)n {
  OSStatus erc = noErr;
  NSLog (@"MIDI object removed: parent=%" PRIu32 ", parentType=%@, child=%" PRIu32 ", childType=%@",
         n->parent, midiTypeName (n->parentType), n->child, midiTypeName (n->childType));
  if (n->childType == kMIDIObjectType_Source && n->child == self->source_) {
    NSLog (@"Our MIDI source was removed!");
    erc = [self disconnectFromMIDISource];
  }
  return erc;
}

- (OSStatus)midiObjectAddedNotification:(MIDIObjectAddRemoveNotification const *)n {
  OSStatus erc = noErr;
  NSLog (@"MIDI object added: parent=%" PRIu32 ", parentType=%@, child=%" PRIu32 ", childType=%@",
         n->parent, midiTypeName (n->parentType), n->child, midiTypeName (n->childType));
  if (n->childType == kMIDIObjectType_Source && self->source_ == 0) {
    MIDIUniqueID uniqueID = 0;
    erc = ::MIDIObjectGetIntegerProperty (n->child, kMIDIPropertyUniqueID, &uniqueID);
    if (erc != noErr) {
      NSLog (@"midiChangedNotification.MIDIObjectGetIntegerProperty: returned %d", erc);
    } else {
      erc = [self connectToMIDIUniqueID:uniqueID];
    }
  }
  return erc;
}

- (void)midiChangedNotification:(MIDINotification const *)message {
  switch (message->messageID) {
    // Some aspect of the current MIDISetup has changed.  No data.  Should ignore this message if
    // the other change notification messages are handled.
    case kMIDIMsgSetupChanged:
      break;
    case kMIDIMsgObjectRemoved:
      if (message->messageSize != sizeof (MIDIObjectAddRemoveNotification)) {
        NSLog (@"MIDI Object removed: Message size was %u, expected %zu", message->messageSize,
               sizeof (MIDIObjectAddRemoveNotification));
      } else {
        [self
            midiObjectRemovedNotificaton:reinterpret_cast<MIDIObjectAddRemoveNotification const *> (
                                             message)];
      }
      break;
    case kMIDIMsgObjectAdded:
      if (message->messageSize != sizeof (MIDIObjectAddRemoveNotification)) {
        NSLog (@"MIDI Object Added: Message size was %u, expected %zu", message->messageSize,
               sizeof (MIDIObjectAddRemoveNotification));
      } else {
        [self
            midiObjectAddedNotification:reinterpret_cast<MIDIObjectAddRemoveNotification const *> (
                                            message)];
      }
      break;
    case kMIDIMsgPropertyChanged:
      NSLog (@"MIDI Property Changed");
      break;
    case kMIDIMsgThruConnectionsChanged:
      NSLog (@"MIDI Through Connections Changed");
      break;
    case kMIDIMsgSerialPortOwnerChanged:
      NSLog (@"MIDI Serial Port Owner Changed");
      break;
    case kMIDIMsgIOError:
      NSLog (@"MIDI IO Error");
      break;
    default:
      NSLog (@"MIDI Change Notified (unknown message ID)");
      break;
  }
}

static void midiChanged (MIDINotification const *const message, void *refCon) {
  if (AppDelegate *const ad = (__bridge AppDelegate *)refCon) {
    [ad midiChangedNotification:message];
  }
}

- (OSStatus)startMIDI {
  MIDIClientRef client = 0;
  OSStatus erc = MIDIClientCreate (CFSTR ("MIDI Client"), midiChanged,
                                   (__bridge void *)self /*notifyRefCon*/, &client);
  if (erc != noErr) {
    NSLog (@"MIDIClientCreate: %u", erc);
  } else {
    erc = MIDIInputPortCreate (client, CFSTR ("MIDI Input Port"), readMIDI, nullptr /*refCon*/,
                               &self->port_);
    if (erc != noErr) {
      NSLog (@"MIDIInputPortCreate: %u", erc);
    }
  }

  if (erc == noErr) {
    ItemCount const sources = MIDIGetNumberOfSources ();
    NSLog (@"There are %u sources", static_cast<unsigned> (sources));
    if (sources == 0) {
      NSLog (@"No MIDI source was available");
      return noErr;
    }

    self->source_ = MIDIGetSource (0);

    CFStringRef pname = Nil;
    MIDIObjectGetStringProperty (self->source_, kMIDIPropertyName, &pname);
    NSLog (@"Connected to: %@", pname);
    CFRelease (pname);

    erc = MIDIPortConnectSource (self->port_, self->source_, (__bridge void *)self /*connRefCon*/);
    if (erc != noErr) {
      NSLog (@"MIDIPortConnectSource: %d", erc);
    }
  }

  return erc;
}

// callback
// ~~~~~~~~
static void callback (void *__nullable userData, AudioQueueRef queue, AudioQueueBufferRef buffer) {
  if (AppDelegate *const ad = (__bridge AppDelegate *)userData) {
    [ad enqueueAudioBuffer:buffer];
  }
}

// application did finish launching
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if TARGET_OS_OSX
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
  NSApplication *const application = [NSApplication sharedApplication];
#elif TARGET_OS_IOS
- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
#endif
  running_ = YES;

  AudioStreamBasicDescription const description = audioDescription ();
  OSStatus erc = ::AudioQueueNewOutput (&description, callback, (__bridge void *)self,
                                        nullptr /*callback run loop*/,
                                        nullptr /*callback run loop mode*/, 0 /*flags*/, &queue_);
  if (erc == noErr) {
    buffers_ =
        static_cast<AudioQueueBufferRef *> (calloc (numBuffers, sizeof (AudioQueueBufferRef)));
    for (unsigned ctr = 0U; ctr < numBuffers && erc == noErr; ++ctr) {
      erc = ::AudioQueueAllocateBuffer (queue_, bufferSize, &buffers_[ctr]);
      if (erc != noErr) {
        NSLog (@"AudioQueueAllocateBuffer returned %d", static_cast<int> (erc));
      } else {
        // prime this buffer
        erc = [self enqueueAudioBuffer:buffers_[ctr]];
      }
    }
  }

  if (erc == noErr) {
    UInt32 framesPrepared = 0;
    erc = ::AudioQueuePrime (queue_, framesPrepared, &framesPrepared);
    if (erc != noErr) {
      NSLog (@"AudioQueuePrime returned %d", static_cast<int> (erc));
    }
  }

  if (erc == noErr) {
    erc = ::AudioQueueStart (queue_, nullptr /* start time */);
    if (erc != noErr) {
      NSLog (@"AudioQueueStart returned %d", static_cast<int> (erc));
    }
  }

  if (erc == noErr) {
    erc = [self startMIDI];
  }

  if (erc != noErr) {
    [self showError:erc];
#if TARGET_OS_OSX
    [application terminate:self];
#elif TARGET_OS_IOS
    return NO;
#endif
  }

#if 0
  dispatch_async (dispatch_get_global_queue (QOS_CLASS_USER_INITIATED, 0), ^{});
#endif

#if TARGET_OS_IOS
  return YES;
#endif
}

// application will terminate
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
#if TARGET_OS_OSX
- (void)applicationWillTerminate:(NSNotification *)notification {
#elif TARGET_OS_IOS
- (void)applicationWillTerminate:(UIApplication *)application {
#endif

  if ([lock_ lockBeforeDate:[NSDate dateWithTimeIntervalSinceNow:lockWaitTime]]) {
    running_ = NO;
    [lock_ unlock];
  } else {
    NSLog (@"applicationWillTerminate could not obtain the lock in a reasonable time!");
  }
  stopAudio (queue_);
  queue_ = nil;
}

// application supports secure restorable state
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if TARGET_OS_OSX
- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app {
  return YES;
}
#endif

// set waveform
// ~~~~~~~~~~~~
- (void)setWaveform:(NSInteger)tag {
  synth::wavetable const *w = nullptr;
  NSString *name = @"";
  switch (tag) {
    case 0:
      name = @"sine";
      w = &synth::sine;
      break;
    case 1:
      name = @"square";
      w = &synth::square;
      break;
    case 2:
      name = @"triangle";
      w = &synth::triangle;
      break;
    case 3:
      name = @"sawtooth=";
      w = &synth::sawtooth;
      break;
  }
  if (w == nullptr) {
    NSLog (@"setWaveform didn't understand waveform tag %ld", tag);
    return;
  }
  NSLog (@"setting waveform to %@", name);

  if (![lock_ lockBeforeDate:[NSDate dateWithTimeIntervalSinceNow:lockWaitTime]]) {
    NSLog (@"setWaveform could not obtain the lock in a reasonable time!");
    return;
  }
  voices_->set_wavetable (w);
  [lock_ unlock];
}

// set frequency
// ~~~~~~~~~~~~~
- (void)setFrequency:(double)f {
  NSLog (@"setFrequency %lf", f);
  if (![lock_ lockBeforeDate:[NSDate dateWithTimeIntervalSinceNow:lockWaitTime]]) {
    NSLog (@"setFrequency could not obtain the lock in a reasonable time!");
    return;
  }
  // osc_->set_frequency (synth::oscillator::frequency::fromfp (f));
  [lock_ unlock];
}

- (void)setEnvelopeStage:(synth::envelope::phase)stage to:(double)value {
  NSString *names[5] = {@"idle", @"attack", @"decay", @"sustain", @"release"};
  NSLog (@"Envelope %@ %f", names[static_cast<int> (stage)], value);

  if (![lock_ lockBeforeDate:[NSDate dateWithTimeIntervalSinceNow:lockWaitTime]]) {
    NSLog (@"setEnvelopeStage:to: could not obtain the lock in a reasonable time!");
    return;
  }
  voices_->set_envelope (stage, value);
  [lock_ unlock];
}

#pragma mark - UISceneSession lifecycle

#if TARGET_OS_OSX
#elif TARGET_OS_IOS
- (UISceneConfiguration *)application:(UIApplication *)application
    configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession
                                   options:(UISceneConnectionOptions *)options {
  // Called when a new scene session is being created.
  // Use this method to select a configuration to create the new scene with.
  return [[UISceneConfiguration alloc] initWithName:@"Default Configuration"
                                        sessionRole:connectingSceneSession.role];
}

- (void)application:(UIApplication *)application
    didDiscardSceneSessions:(NSSet<UISceneSession *> *)sceneSessions {
  // Called when the user discards a scene session.
  // If any sessions were discarded while the application was not running, this
  // will be called shortly after application:didFinishLaunchingWithOptions.
  // Use this method to release any resources that were specific to the
  // discarded scenes, as they will not return.
}
#else
#error "Unknown target"
#endif

@end
