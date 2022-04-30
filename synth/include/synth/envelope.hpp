// -*- mode: c++; coding: utf-8-unix; -*-
#ifndef SYNTH_ENVELOPE_HPP
#define SYNTH_ENVELOPE_HPP

#include <cmath>

#include "nco.hpp"

namespace synth {

class envelope {
public:
  using amplitude = wavetable::amplitude;

  void note_on ();
  void note_off ();
  bool active () const;

  enum class phase { idle, attack, decay, sustain, release };

  void set (phase p, double value);

  amplitude tick (amplitude const v);

private:
  double attack_ = 0.0;
  double decay_ = 0.0;
  double sustain_ = 1.0;
  double release_ = 0.0;

  phase phase_ = phase::idle;
  amplitude a_;

  static unsigned time (double seconds) {
    return static_cast<unsigned> (
        std::round (oscillator::sample_rate * seconds));
  }
};

}  // end namespace synth

#endif  // SYNTH_ENVELOPE_HPP
