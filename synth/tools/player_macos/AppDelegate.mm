#import "AppDelegate.h"

#import <AudioToolbox/AudioToolbox.h>

#include "synth/nco.hpp"

using SampleType = Float32;
static constexpr UInt32 buffer_size = 0x1000;
static constexpr auto numBuffers = 8U;
static constexpr NSTimeInterval lockWaitTime = 1.0;

static AudioQueueRef createAudioQueue (AudioQueueOutputCallback callback, void *userData) {
  AudioStreamBasicDescription d = {0};
  d.mSampleRate = static_cast<Float64> (synth::oscillator::sample_rate);
  d.mFormatID = kAudioFormatLinearPCM;
  d.mFormatFlags = kLinearPCMFormatFlagIsFloat;
  d.mBytesPerPacket = UInt32{sizeof (SampleType)};
  d.mFramesPerPacket = UInt32{1};
  d.mBytesPerFrame = d.mBytesPerPacket * d.mFramesPerPacket;
  d.mChannelsPerFrame = UInt32{1};
  d.mBitsPerChannel = UInt32{8 * sizeof (SampleType)};

  AudioQueueRef queue = nil;
  OSStatus erc =
      ::AudioQueueNewOutput (&d, callback, userData /* user data */, nullptr /*callback run loop*/,
                             nullptr /*callback run loop mode*/, 0 /*flags*/, &queue);
  NSLog (@"AudioQueueNewOutput erc=%d", static_cast<int> (erc));
  return queue;
}

static AudioQueueBufferRef *allocateBuffers (AudioQueueRef queue) {
  auto *buffers =
      static_cast<AudioQueueBufferRef *> (malloc (sizeof (AudioQueueBufferRef) * numBuffers));
  for (unsigned ctr = 0U; ctr < numBuffers; ++ctr) {
    AudioQueueBufferRef buffer = nullptr;
    OSStatus const erc = ::AudioQueueAllocateBuffer (queue, buffer_size, &buffer);
    NSLog (@"AudioQueueAllocateBuffer erc=%d", static_cast<int> (erc));
    buffers[ctr] = buffer;
  }
  return buffers;
}

@interface AppDelegate () {
  std::unique_ptr<synth::wavetable> sine_;
  std::unique_ptr<synth::wavetable> square_;
  std::unique_ptr<synth::wavetable> triangle_;

  std::unique_ptr<synth::oscillator> osc_;

  Boolean running_;
  AudioQueueRef queue_;
  AudioQueueBufferRef *buffers_;
  NSLock *lock_;
}
@end

@implementation AppDelegate

// enqueue audio buffer
// ~~~~~~~~~~~~~~~~~~~~
- (void)enqueueAudioBuffer:(AudioQueueBufferRef)buffer {
  UInt32 samples = buffer->mAudioDataBytesCapacity / sizeof (SampleType);
  auto *const first = static_cast<SampleType *> (buffer->mAudioData);
  auto *const last = first + samples;

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
    if (OSStatus const erc = ::AudioQueueEnqueueBuffer (queue_, buffer, 0U, nullptr)) {
      NSLog (@"AudioQueueEnqueueBuffer error %d", erc);
    }
  }
}

// callback
// ~~~~~~~~
static void callback (void *__nullable userData, AudioQueueRef queue, AudioQueueBufferRef buffer) {
  if (AppDelegate *const ad = static_cast<AppDelegate *> (userData)) {
    [ad enqueueAudioBuffer:buffer];
  }
}

// init
// ~~~~
- (id)init {
  self = [super init];
  if (self) {
    queue_ = createAudioQueue (&callback, self);
    buffers_ = allocateBuffers (queue_);
    lock_ = [NSLock new];
    running_ = NO;
    // ...
  }
  return self;
}

static void showError (OSStatus const erc) {
  [[NSAlert alertWithError:[NSError errorWithDomain:NSOSStatusErrorDomain code:erc
                                           userInfo:nil]] runModal];
}

static constexpr inline double sine (double const theta) { return std::sin (theta); }
static constexpr inline double square (double const theta) {
  return theta <= synth::pi ? 1.0 : -1.0;
}
static constexpr inline double triangle (double const theta) {
  return (theta <= synth::pi ? theta : (synth::two_pi - theta)) / synth::half_pi - 1.0;
}

// application did finish launching
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
  if (![lock_ lockBeforeDate:[NSDate dateWithTimeIntervalSinceNow:lockWaitTime]]) {
    NSLog (@"applicationDidFinishLaunching could not obtain the lock in a reasonable time!");
  }

  sine_.reset (new synth::wavetable{sine});
  square_.reset (new synth::wavetable{square});
  triangle_.reset (new synth::wavetable{triangle});

  osc_.reset (new synth::oscillator (sine_.get ()));
  osc_->set_frequency (synth::oscillator::frequency::fromfp (440.0));

  running_ = YES;
  [lock_ unlock];

  // prime the buffers
  for (unsigned ctr = 0U; ctr < numBuffers; ++ctr) {
    [self enqueueAudioBuffer:buffers_[ctr]];
  }

  UInt32 framesPrepared = 0;
  if (OSStatus const erc = ::AudioQueuePrime (queue_, framesPrepared, &framesPrepared)) {
    NSLog (@"AudioQueuePrime returned %d", static_cast<int> (erc));
    showError (erc);
    [[NSApplication sharedApplication] terminate:self];
    return;
  }

  if (OSStatus const erc = ::AudioQueueStart (queue_, nullptr /* start time */)) {
    NSLog (@"AudioQueueStart returned %d", static_cast<int> (erc));
    showError (erc);
    [[NSApplication sharedApplication] terminate:self];
    return;
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

  OSStatus erc = ::AudioQueueFlush (queue_);
  NSLog (@"AudioQueueFlush returned %d", static_cast<int> (erc));
  erc = ::AudioQueueStop (queue_, true /*immediate*/);
  NSLog (@"AudioQueueStop returned %d", static_cast<int> (erc));
  erc = ::AudioQueueDispose (queue_, true /*immediate*/);
  NSLog (@"AudioQueueDispose returned %d", static_cast<int> (erc));
}

// application supports secure restorable state
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app {
  return YES;
}

// set waveform
// ~~~~~~~~~~~~
- (void)setWaveform:(NSInteger)tag {
  synth::wavetable *w = nullptr;
  NSString *name = @"";
  switch (tag) {
    case 0:
      name = @"sine";
      w = sine_.get ();
      break;
    case 1:
      name = @"square";
      w = square_.get ();
      break;
    case 2:
      name = @"triangle";
      w = triangle_.get ();
      break;
  }
  if (w == nullptr) {
    NSLog (@"setWaveform didn't understand waveform tag %ld", tag);
    return;
  }

  if (![lock_ lockBeforeDate:[NSDate dateWithTimeIntervalSinceNow:lockWaitTime]]) {
    NSLog (@"setWaveform could not obtain the lock in a reasonable time!");
    return;
  }
  NSLog (@"setting waveform to %@", name);
  osc_->set_wavetable (w);
  [lock_ unlock];
}

// set frequency
// ~~~~~~~~~~~~~
- (void)setFrequency:(double)f {
  if (![lock_ lockBeforeDate:[NSDate dateWithTimeIntervalSinceNow:lockWaitTime]]) {
    NSLog (@"setFrequency could not obtain the lock in a reasonable time!");
    return;
  }
  NSLog (@"setFrequency %lf", f);
  osc_->set_frequency (synth::oscillator::frequency::fromfp (f));
  [lock_ unlock];
}

@end
