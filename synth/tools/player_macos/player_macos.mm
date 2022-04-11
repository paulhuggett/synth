#include <iostream>
#include <limits>
#include <vector>

#include "synth/nco.hpp"

#import "audio_queue.h"
#import "error.h"

class player {
public:
  static constexpr UInt32 buffer_size = 0x1000;
  static constexpr auto num_buffers = 8U;

  player ();

  void run ();

  bool done () const { return is_done_; }
  void stop () { is_done_ = true; }

  void wait () {
    std::unique_lock<std::mutex> lock{mut_};
    while (!done ()) {
      cv_.wait (lock);
    }
  }

private:
  using sample_type = float;

  void callback (AudioQueueRef queue, AudioQueueBufferRef buffer);

  static void aq_callback (void *user_data, AudioQueueRef queue, AudioQueueBufferRef buffer) {
    static_cast<player *> (user_data)->callback (queue, buffer);
  }
  static AudioStreamBasicDescription description ();
  std::vector<AudioQueueBufferRef> allocate_buffers (AudioQueueRef queue);
  void preroll (AudioQueueRef queue, std::vector<AudioQueueBufferRef> *NONNULL const buffers);

  bool is_done_ = false;
  std::mutex mut_;
  std::condition_variable cv_;

  synth::wavetable const sine_;
  synth::oscillator osc_;
};

// (ctor)
// ~~~~~~
player::player () : sine_{[] (double const theta) { return std::sin (theta); }}, osc_{&sine_} {
  osc_.set_frequency (synth::oscillator::frequency::fromfp (440.0));
}

// run
// ~~~
void player::run () {
  audio_queue::qptr queue =
      audio_queue::new_output (description (), &aq_callback, this, nullptr, nullptr, 0);
  std::vector<AudioQueueBufferRef> buffers = player::allocate_buffers (queue.get ());
  this->preroll (queue.get (), &buffers);

  if (!this->done ()) {
    audio_queue::prime (queue, 0U);
    audio_queue::start (queue);

    this->wait ();

    audio_queue::flush (queue);
    audio_queue::stop (queue, true);
  }
}

// callback
// ~~~~~~~~
void player::callback (AudioQueueRef queue, AudioQueueBufferRef buffer) {
  auto *const start = static_cast<sample_type *> (buffer->mAudioData);
  auto *const end = std::generate_n (start, buffer->mAudioDataBytesCapacity / sizeof (sample_type),
                                     [this] { return osc_.tick ().as_double (); });
  buffer->mAudioDataByteSize = (end - start) * sizeof (sample_type);

  if (OSStatus const erc = ::AudioQueueEnqueueBuffer (queue, buffer, 0U, nullptr)) {
    NSLog (@"AudioQueueEnqueueBuffer error %d", erc);
    this->stop ();
  }
}

// description
// ~~~~~~~~~~~
AudioStreamBasicDescription player::description () {
  AudioStreamBasicDescription d = {0};
  d.mSampleRate = static_cast<Float64> (synth::oscillator::sample_rate);
  d.mFormatID = kAudioFormatLinearPCM;
  d.mFormatFlags = kLinearPCMFormatFlagIsFloat;
  d.mBytesPerPacket = UInt32{sizeof (sample_type)};
  d.mFramesPerPacket = UInt32{1};
  d.mBytesPerFrame = d.mBytesPerPacket * d.mFramesPerPacket;
  d.mChannelsPerFrame = UInt32{1};
  d.mBitsPerChannel = UInt32{8 * sizeof (sample_type)};
  return d;
}

// allocate buffers [static]
// ~~~~~~~~~~~~~~~~
std::vector<AudioQueueBufferRef> player::allocate_buffers (AudioQueueRef queue) {
  std::vector<AudioQueueBufferRef> buffers;
  buffers.reserve (num_buffers);
  std::generate_n (std::back_inserter (buffers), num_buffers,
                   [&] { return audio_queue::allocate_buffer (queue, buffer_size); });
  return buffers;
}

// preroll
// ~~~~~~~
void player::preroll (AudioQueueRef queue, std::vector<AudioQueueBufferRef> *const buffers) {
  // Fill the buffers with initial data.
  for (AudioQueueBufferRef &buffer : *buffers) {
    this->callback (queue, buffer);  // The queueing of the buffer is handled by the callback.
    if (this->done ()) {
      break;
    }
  }
}

int main (int argc, const char *argv[]) {
  int exit_code = EXIT_SUCCESS;
  @autoreleasepool {
    try {
      player p;
      p.run ();
    } catch (std::exception const &ex) {
      std::cerr << "Error: " << ex.what () << std::endl;
      exit_code = EXIT_FAILURE;
    } catch (...) {
      std::cerr << "Unknown error" << std::endl;
      exit_code = EXIT_FAILURE;
    }
  }
  return exit_code;
}
