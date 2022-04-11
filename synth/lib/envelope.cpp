#include "synth/envelope.hpp"

namespace synth {

void envelope::note_on () { phase_ = phase::attack; }

void envelope::note_off () {
  time_ = time (release_time);
  phase_ = phase::release;
}

auto envelope::tick (amplitude const v) -> amplitude {
  switch (phase_) {
    case phase::attack:
      if (time_ < oscillator::sample_rate * attack_time) {
        return amplitude::fromfp (v.as_double () * (time_++ * attack));
      }
      phase_ = phase::decay;
      time_ = time (decay_time);
      // FALLTHROUGH

    case phase::decay:
      if (time_ > 0U) {
        return amplitude::fromfp (v.as_double () * (--time_ * decay + sustain));
      }
      phase_ = phase::sustain;
      // FALLTHROUGH
    case phase::sustain:
      return amplitude::fromfp (v.as_double () * sustain);

    case phase::release:
      if (time_ > 0U) {
        return amplitude::fromfp (v.as_double () *
                                  (--time_ * release - sustain));
      }
      phase_ = phase::done;
      // FALLTHROUGH

    case phase::done:
      break;
  }
  time_ = 0U;
  return amplitude::fromint (0U);
}

}  // end namespace synth
