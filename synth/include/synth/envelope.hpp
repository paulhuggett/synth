// -*- mode: c++; coding: utf-8-unix; -*-
#ifndef SYNTH_ENVELOPE_HPP
#define SYNTH_ENVELOPE_HPP

#include <cmath>
#include <cstdint>

#include "nco.hpp"

namespace synth {

class envelope {
public:
  using amplitude = wavetable::amplitude;

  void note_on ();
  void note_off ();
  bool active () const;

  // Set the bottom bit for time-based envelope phases (i.e. ADR).
  static constexpr auto timed_phase_mask = uint8_t{0b001};
  enum class phase : uint8_t {
    idle = 0b000,
    attack = 0b000 | timed_phase_mask,
    decay = 0b010 | timed_phase_mask,
    sustain = 0b010,
    release = 0b100 | timed_phase_mask,
  };
  static char const* NONNULL phase_name (phase p) noexcept;

  void set (phase p, double v);

  amplitude tick (amplitude v);

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
