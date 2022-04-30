// -*- mode: c++; coding: utf-8-unix; -*-
#include "synth/envelope.hpp"

namespace synth {

void envelope::note_on () { phase_ = phase::attack; }

void envelope::note_off () {
  phase_ = phase::release;
}

bool envelope::active () const { return phase_ != phase::idle; }

void envelope::set (phase p, double value) {
  if (!std::isfinite (value)) {
    return;
  }
  value = std::max (value, 0.0);
  auto const delta_value = [] (double const v) {
    return v > 0.0 ? 1.0 / (v * oscillator::sample_rate) : 0.0;
  };
  switch (p) {
    case phase::idle:
      break;
    case phase::attack:
      attack_ = delta_value (value);
      break;
    case phase::decay:
      decay_ = delta_value (value);
      break;
    case phase::release:
      release_ = delta_value (value);
      break;
    case phase::sustain:
      sustain_ = std::min (1.0, value);
      break;
  }
}

auto envelope::tick (amplitude const v) -> amplitude {
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
      // FALLTHROUGH
    case phase::decay:
      if (decay_ <= 0.0) {
        a_ = amplitude::fromfp (sustain_);
      } else if (a_.as_double () > sustain_) {
        delta = -decay_;
        break;
      }
      phase_ = phase::sustain;
      // FALLTHROUGH
    case phase::sustain:
      return amplitude::fromfp (v.as_double () * sustain_);

    case phase::release:
      if (release_ <= 0) {
        a_ = amplitude::fromfp (0.0);
      } else if (a_.as_double () > 0.0) {
        delta = -release_;
        break;
      }
      phase_ = phase::idle;
      // FALLTHROUGH
    case phase::idle:
      return amplitude::fromfp (0.0);
  }

  a_ = amplitude::fromfp (a_.as_double () + delta);
  return amplitude::fromfp (v.as_double () * a_.as_double ());
}

}  // end namespace synth
