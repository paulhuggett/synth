#ifndef AUDIO_QUEUE_H
#define AUDIO_QUEUE_H

#import <AudioToolbox/AudioToolbox.h>

#include <memory>

namespace audio_queue {

using qptr = std::unique_ptr<std::pointer_traits<AudioQueueRef>::element_type,
                             void (*) (AudioQueueRef __nullable)>;

qptr new_output (AudioStreamBasicDescription const &format,
                 AudioQueueOutputCallback __nonnull callback,
                 void *__nullable user_data,
                 CFRunLoopRef __nullable callback_run_loop,
                 CFStringRef __nullable run_loop_mode, UInt32 flags);

void set_property (AudioQueueRef __nonnull queue, AudioQueuePropertyID property,
                   void const *_Nonnull data, UInt32 data_size);
void set_property (qptr &queue, AudioQueuePropertyID property,
                   void const *_Nonnull data, UInt32 data_size);
template <typename T>
void set_property (AudioQueueRef __nonnull queue, AudioQueuePropertyID property,
                   T const &data) {
  set_property (queue, property, data);
}
template <typename T>
void set_property (qptr &queue, AudioQueuePropertyID property, T const &data) {
  set_property (queue.get (), property, data);
}

AudioQueueBufferRef __nonnull allocate_buffer (AudioQueueRef __nonnull queue,
                                               UInt32 const buffer_size);
AudioQueueBufferRef __nonnull allocate_buffer (qptr &q,
                                               UInt32 const buffer_size);

UInt32 prime (AudioQueueRef __nonnull queue, UInt32 const frames_to_prepare);
UInt32 prime (qptr &q, UInt32 const frames_to_prepare);

void start (AudioQueueRef __nonnull queue,
            AudioTimeStamp const *const __nullable start_time = nullptr);
void start (qptr &q,
            AudioTimeStamp const *const __nullable start_time = nullptr);

void flush (AudioQueueRef __nonnull queue);
void flush (qptr &q);

void stop (AudioQueueRef __nonnull queue, bool const immediate);
void stop (qptr &q, bool const immediate);

}  // end namespace audio_queue

#endif  // AUDIO_QUEUE_H
