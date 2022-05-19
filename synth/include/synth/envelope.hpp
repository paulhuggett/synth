// -*- mode: c++; coding: utf-8-unix; -*-
#ifndef SYNTH_ENVELOPE_HPP
#define SYNTH_ENVELOPE_HPP

#include <cmath>
#include <cstdint>
#include <type_traits>

#include "nco.hpp"

namespace synth {

template <unsigned SampleRate>
class envelope {
public:
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
    return static_cast<unsigned> (std::round (SampleRate * seconds));
  }
};

// note on
// ~~~~~~~
template <unsigned SampleRate>
void envelope<SampleRate>::note_on () {
  phase_ = phase::attack;
}

// note off
// ~~~~~~~~
template <unsigned SampleRate>
void envelope<SampleRate>::note_off () {
  phase_ = phase::release;
}

template <unsigned SampleRate>
bool envelope<SampleRate>::active () const {
  return phase_ != phase::idle;
}

template <unsigned SampleRate>
void envelope<SampleRate>::set (phase p, double v) {
  if (!std::isfinite (v)) {
    return;
  }
  v = std::max (v, 0.0);
  if (static_cast<std::underlying_type_t<decltype (p)>> (p) &
      timed_phase_mask) {
    v = v > 0.0 ? 1.0 / (v * SampleRate) : 0.0;
  }
  switch (p) {
    case phase::idle: break;
    case phase::attack: attack_ = v; break;
    case phase::decay: decay_ = v; break;
    case phase::sustain: sustain_ = std::min (v, 1.0); break;
    case phase::release: release_ = v; break;
  }
}

template <unsigned SampleRate>
char const* NONNULL envelope<SampleRate>::phase_name (phase const p) noexcept {
  switch (p) {
    case phase::idle: return "idle";
    case phase::sustain: return "sustain";
    case phase::attack: return "attack";
    case phase::decay: return "decay";
    case phase::release: return "release";
  }
  return "";
}

template <unsigned SampleRate>
auto envelope<SampleRate>::tick (amplitude const v) -> amplitude {
  auto delta = 0.0;

  switch (phase_) {
    case phase::attack:
      if (attack_ <= 0.0) {
        a_ = amplitude::fromfp (1.0);
      } else if (a_.as_double () < 1.0) {
        delta = attack_;
        break;
      }
      phase_ = phase::decay;
      [[fallthrough]];
    case phase::decay:
      if (decay_ <= 0.0) {
        a_ = amplitude::fromfp (sustain_);
      } else if (a_.as_double () > sustain_) {
        delta = -decay_;
        break;
      }
      phase_ = phase::sustain;
      [[fallthrough]];
    case phase::sustain: return amplitude::fromfp (v.as_double () * sustain_);

    case phase::release:
      if (release_ <= 0) {
        a_ = amplitude::fromfp (0.0);
      } else if (a_.as_double () > 0.0) {
        delta = -release_;
        break;
      }
      phase_ = phase::idle;
      [[fallthrough]];
    case phase::idle: return amplitude::fromfp (0.0);
  }

  a_ = amplitude::fromfp (a_.as_double () + delta);
  return amplitude::fromfp (v.as_double () * a_.as_double ());
}

}  // end namespace synth

#endif  // SYNTH_ENVELOPE_HPP
