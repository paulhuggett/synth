// -*- mode: c++; coding: utf-8-unix; -*-
#ifndef POLY_BLEP_SAWTOOTH
#define POLY_BLEP_SAWTOOTH

#include <cassert>
#include <cmath>

#include "synth/wavetable.hpp"

template <unsigned SampleRate>
class poly_blep_sawtooth_oscillator {
public:
  static constexpr const auto sample_rate = SampleRate;

  void set_frequency (synth::frequency const f) {
    dt_ = f.as_double () / sample_rate;
  }

  synth::amplitude tick () {
    t_ += dt_;
    if (t_ >= 1.0) {
      t_ -= 1.0;
    }
    return synth::amplitude::fromfp (poly_saw (t_, dt_));
  }

private:
  double dt_ = 0.0;
  double t_ = 0.0;

  static constexpr double naive_saw (double t) { return 2.0 * t - 1.0; }

  static constexpr double poly_saw (double t, double const dt) {
    assert (std::isfinite (t) && t >= 0.0 && t < 1.0);
    // Correct phase, so it would be in line with sin(2.0*M_PI * t)
    t += 0.5;
    if (t >= 1.0) {
      t -= 1.0;
    }
    return poly_blep_sawtooth_oscillator::naive_saw (t) - poly_blep (t, dt);
  }
  static constexpr double poly_blep (double t, double const dt) {
    // t - t^2/2 + 1/2, 0 <= t < 1
    if (t < dt) {
      t /= dt;
      return (t + t) - (t * t) - 1.0;
    }
    // t^2/2 +t +1/2, -1 < t < 0
    if (t > 1.0 - dt) {
      t = (t - 1.0) / dt;
      return (t * t) + (t + t) + 1.0;
    }
    // 0 otherwise
    return 0.0;
  }
};

#endif  // POLY_BLEP_SAWTOOTH
