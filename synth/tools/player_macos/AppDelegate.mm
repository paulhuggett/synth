#import "AppDelegate.h"

#import <AudioToolbox/AudioToolbox.h>

#include "synth/nco.hpp"

using SampleType = Float32;
static constexpr UInt32 bufferSize = 0x1000;
static constexpr auto numBuffers = 8U;
static constexpr NSTimeInterval lockWaitTime = 1.0;


// audio description
// ~~~~~~~~~~~~~~~~~
AudioStreamBasicDescription audioDescription (void) {
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

// show error
// ~~~~~~~~~~
static void showError (OSStatus const erc) {
  [[NSAlert alertWithError:[NSError errorWithDomain:NSOSStatusErrorDomain code:erc
                                           userInfo:nil]] runModal];
}

@interface AppDelegate () {
  std::unique_ptr<synth::oscillator> osc_;

  Boolean running_;
  AudioQueueRef queue_;
  AudioQueueBufferRef *buffers_;
  NSLock *lock_;
}
@end

@implementation AppDelegate

// init
// ~~~~
- (id)init {
  self = [super init];
  if (self) {
    queue_ = nil;
    buffers_ = nil;
    lock_ = [NSLock new];
    running_ = NO;
    osc_.reset (new synth::oscillator (&synth::sine));
    osc_->set_frequency (synth::oscillator::frequency::fromfp (440.0));
    // ...
  }
  return self;
}

// enqueue audio buffer
// ~~~~~~~~~~~~~~~~~~~~
- (OSStatus)enqueueAudioBuffer:(AudioQueueBufferRef)buffer {
  UInt32 samples = buffer->mAudioDataBytesCapacity / sizeof (SampleType);
  auto *const first = static_cast<SampleType *> (buffer->mAudioData);
  auto *const last = first + samples;

  OSStatus erc = noErr;
  Boolean running = NO;
  if ([lock_ lockBeforeDate:[NSDate dateWithTimeIntervalSinceNow:lockWaitTime]]) {
    running = running_;
    if (running) {
      for (auto *it = first; it != last; ++it) {
        *it = osc_->tick ().as_double ();
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

// callback
// ~~~~~~~~
static void callback (void *__nullable userData, AudioQueueRef queue, AudioQueueBufferRef buffer) {
  if (AppDelegate *const ad = (__bridge AppDelegate *)userData) {
    [ad enqueueAudioBuffer:buffer];
  }
}

// application did finish launching
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
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
      if (erc == noErr) {
        // prime this buffer
        erc = [self enqueueAudioBuffer:buffers_[ctr]];
      }
    }
  }

  if (erc == noErr) {
    UInt32 framesPrepared = 0;
    erc = ::AudioQueuePrime (queue_, framesPrepared, &framesPrepared);
  }

  if (erc == noErr) {
    erc = ::AudioQueueStart (queue_, nullptr /* start time */);
  }

  if (erc != noErr) {
    NSLog (@"AudioQueueStart returned %d", static_cast<int> (erc));
    showError (erc);
    [[NSApplication sharedApplication] terminate:self];
  }
}

// application will terminate
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
- (void)applicationWillTerminate:(NSNotification *)aNotification {
  if ([lock_ lockBeforeDate:[NSDate dateWithTimeIntervalSinceNow:lockWaitTime]]) {
    running_ = NO;
    [lock_ unlock];
  } else {
    NSLog (@"applicationWillTerminate could not obtain the lock in a reasonable time!");
  }

  if (OSStatus const erc = ::AudioQueueFlush (queue_)) {
    NSLog (@"AudioQueueFlush returned %d", static_cast<int> (erc));
  }
  if (OSStatus const erc = ::AudioQueueStop (queue_, true /*immediate*/)) {
    NSLog (@"AudioQueueStop returned %d", static_cast<int> (erc));
  }
  if (OSStatus const erc = ::AudioQueueDispose (queue_, true /*immediate*/)) {
    NSLog (@"AudioQueueDispose returned %d", static_cast<int> (erc));
  }
}

// application supports secure restorable state
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app {
  return YES;
}

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
  osc_->set_wavetable (w);
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
  osc_->set_frequency (synth::oscillator::frequency::fromfp (f));
  [lock_ unlock];
}

@end
