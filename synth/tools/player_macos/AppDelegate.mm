#import "AppDelegate.h"

#import <AudioToolbox/AudioToolbox.h>

#include "synth/voice_assigner.hpp"

using SampleType = Float32;
static constexpr UInt32 bufferSize = 0x1000;
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
  Boolean running = NO;
  if ([lock_ lockBeforeDate:[NSDate dateWithTimeIntervalSinceNow:lockWaitTime]]) {
    running = running_;
    if (running) {
      for (auto *it = first; it != last; ++it) {
        *it = voices_->tick () * masterVolume;
        //        *it = osc_->tick ().as_double () * masterVolume;
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

// play scale
// ~~~~~~~~~~
- (void)playScale {
  static constexpr int scaleMembers = 8;
  static unsigned const majorScale[scaleMembers] = {0U, 2U, 4U, 5U, 7U, 9U, 11U, 12U};
  constexpr auto c4 = 60U;  // (middle C)

  NSLock *const lock = self->lock_;
  NSDate *t = [NSDate now];
  BOOL ok = YES;
  BOOL keyDown = YES;
  int index = 0;
  int direction = -1;
  if ([lock lockBeforeDate:[NSDate dateWithTimeIntervalSinceNow:lockWaitTime]]) {
    while (self->running_ && ok) {
      // lock owned
      unsigned const note = c4 + majorScale[index];
      if (keyDown) {
        NSLog (@"note on %u", note);
        self->voices_->note_on (note);
      } else {
        NSLog (@"note off %u", note);
        self->voices_->note_off (note);
      }
      // lock released
      [lock unlock];

      // update local state
      if (!keyDown) {
        if (index <= 0 || index >= scaleMembers - 1) {
          direction = -direction;
        }
        index += direction;
      }
      keyDown = !keyDown;

      // back to sleep for another iteration.
      t = [NSDate dateWithTimeInterval:0.1 sinceDate:t];
      [NSThread sleepUntilDate:t];
      ok = [lock lockBeforeDate:[NSDate dateWithTimeIntervalSinceNow:lockWaitTime]];
    }

    [lock unlock];
  }
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

  if (erc != noErr) {
    [self showError:erc];
#if TARGET_OS_OSX
    [application terminate:self];
#elif TARGET_OS_IOS
    return NO;
#endif
  }

  dispatch_async (dispatch_get_global_queue (QOS_CLASS_USER_INITIATED, 0), ^{
    [self playScale];
  });

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
