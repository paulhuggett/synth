#include "audio_queue.h"
#include "error.h"

namespace audio_queue {

using qptr =
    std::unique_ptr<std::pointer_traits<AudioQueueRef>::element_type, void (*) (AudioQueueRef)>;

qptr new_output (AudioStreamBasicDescription const &format, AudioQueueOutputCallback callback,
                 void *__nullable user_data, CFRunLoopRef __nullable callback_run_loop,
                 CFStringRef __nullable run_loop_mode, UInt32 flags) {
  AudioQueueRef queue = nullptr;
  if (OSStatus const erc = ::AudioQueueNewOutput (&format, callback, user_data, callback_run_loop,
                                                  run_loop_mode, flags, &queue)) {
    throw std::system_error{make_osstatus_error_code (erc), "AudioQueueNewOutput"};
  }
  return qptr{queue, [] (AudioQueueRef q) { ::AudioQueueDispose (q, true); }};
}

void set_property (AudioQueueRef queue, AudioQueuePropertyID property, void const *data,
                   UInt32 data_size) {
  OSStatus const erc = ::AudioQueueSetProperty (queue, property, data, data_size);
  if (erc != noErr) {
    throw std::system_error{make_osstatus_error_code (erc), "AudioQueueSetProperty"};
  }
}
void set_property (qptr &queue, AudioQueuePropertyID property, void const *data, UInt32 data_size) {
  return set_property (queue.get (), property, data, data_size);
}

AudioQueueBufferRef allocate_buffer (AudioQueueRef queue, UInt32 const buffer_size) {
  AudioQueueBufferRef buffer = nullptr;
  if (OSStatus const erc = ::AudioQueueAllocateBuffer (queue, buffer_size, &buffer)) {
    throw std::system_error{make_osstatus_error_code (erc), "AudioQueueAllocateBuffer"};
  }
  return buffer;
}
AudioQueueBufferRef allocate_buffer (qptr &q, UInt32 const buffer_size) {
  return allocate_buffer (q.get (), buffer_size);
}

UInt32 prime (AudioQueueRef queue, UInt32 const frames_to_prepare) {
  auto frames_prepared = UInt32{0};
  OSStatus const erc = ::AudioQueuePrime (queue, frames_to_prepare, &frames_prepared);
  if (erc != noErr) {
    throw std::system_error{make_osstatus_error_code (erc), "AudioQueuePrime"};
  }
  return frames_prepared;
}

UInt32 prime (qptr &q, UInt32 const frames_to_prepare) {
  return prime (q.get (), frames_to_prepare);
}

void start (AudioQueueRef queue, AudioTimeStamp const *const __nullable start_time) {
  OSStatus const erc = ::AudioQueueStart (queue, start_time);
  if (erc != noErr) {
    throw std::system_error{make_osstatus_error_code (erc), "AudioQueueStart"};
  }
}
void start (qptr &q, AudioTimeStamp const *const __nullable start_time) {
  return start (q.get (), start_time);
}

void flush (AudioQueueRef queue) {
  if (OSStatus const erc = ::AudioQueueFlush (queue)) {
    throw std::system_error{make_osstatus_error_code (erc), "AudioQueueFlush"};
  }
}
void flush (qptr &q) { flush (q.get ()); }

void stop (AudioQueueRef queue, bool const immediate) {
  if (OSStatus const erc = ::AudioQueueStop (queue, immediate)) {
    throw std::system_error{make_osstatus_error_code (erc), "AudioQueueStop"};
  }
}
void stop (qptr &q, bool const immediate) { stop (q.get (), immediate); }

}  // end namespace audio_queue
