// -*- mode: c++; coding: utf-8-unix; -*-
#include "synth/voice.hpp"

#include <numeric>

namespace synth {

void voice::note_on (unsigned const note) {
  auto const f = midi_note_to_frequency (note);
  osc_[0].set_frequency (frequency::fromfp (f));        // 8'
  osc_[1].set_frequency (frequency::fromfp (f + 4.0));
  env_.note_on ();
}

void voice::note_off () { env_.note_off (); }

void voice::set_wavetable (wavetable const* const w) {
  for (auto& osc : osc_) {
    osc.set_wavetable (w);
  }
}

void voice::set_envelope (envelope::phase stage, double value) {
  env_.set (stage, value);
}

auto voice::tick () -> amplitude {
  if (!env_.active ()) {
    return amplitude::fromfp (0.0);
  }
  double const a = std::accumulate (std::begin (osc_), std::end (osc_), 0.0,
                                    [] (double const acc, oscillator& osc) {
                                      return acc + osc.tick ().as_double ();
                                    });
  return env_.tick (amplitude::fromfp (a));
}

}  // end namespace synth
