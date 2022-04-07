// -*- mode: c++; coding: utf-8-unix; -*-
#ifndef SYNTH_ENVELOPE_HPP
#define SYNTH_ENVELOPE_HPP

#include <cmath>

#include "nco.hpp"

namespace synth {

class envelope {
public:
  static constexpr double attack_time = 0.05;
  static constexpr double decay_time = 0.05;
  static constexpr double sustain = 0.5;
  static constexpr double release_time = 0.2;
  using amplitude = wavetable::amplitude;

  void note_on () { phase_ = phase::attack; }
  void note_off () {
    time_ = time (release_time);
    phase_ = phase::release;
  }

  amplitude tick (amplitude const v) {
    switch (phase_) {
      case phase::attack:
        if (time_ < oscillator::sample_rate * attack_time) {
          return amplitude::fromfp (v.as_double () * (time_++ * attack));
        }
        phase_ = phase::decay;
        time_ = time (decay_time);

      case phase::decay:
        if (time_ > 0U) {
          return amplitude::fromfp (v.as_double () *
                                    (--time_ * decay + sustain));
        }
        phase_ = phase::sustain;

      case phase::sustain:
        return amplitude::fromfp (v.as_double () * sustain);

      case phase::release:
        if (time_ > 0U) {
          return amplitude::fromfp (v.as_double () *
                                    (--time_ * release - sustain));
        }
        phase_ = phase::done;

      case phase::done:
        time_ = 0U;
        return amplitude::fromint (0U);
    }
  }

private:
  enum class phase { attack, decay, sustain, release, done };
  static constexpr double attack =
      1.0 / (attack_time * oscillator::sample_rate);
  static constexpr double decay =
      sustain / (decay_time * oscillator::sample_rate);
  static constexpr double release =
      1.0 / (release_time * oscillator::sample_rate);

  phase phase_ = phase::attack;
  unsigned time_ = 0;

  static unsigned time (double seconds) {
    return static_cast<unsigned> (
        std::round (oscillator::sample_rate * seconds));
  }
};

}  // end namespace synth

#endif  // SYNTH_ENVELOPE_HPP
