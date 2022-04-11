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

  void note_on ();
  void note_off ();

  amplitude tick (amplitude const v);

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
