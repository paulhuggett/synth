// -*- mode: c++; coding: utf-8-unix; -*-
#include "synth/voice_assigner.hpp"

#include <numeric>

namespace synth {

voice_assigner::vm::vm () : v{&triangle}, note{unassigned} {}

void voice_assigner::note_on (unsigned const note) {
  if (voices_[next_].note != unassigned) {
    voices_[next_].v.note_off ();
  }
  voices_[next_].note = note;
  voices_[next_].v.note_on (note);
  ++next_;
  if (next_ >= voices_.size ()) {
    next_ = 0U;
  }
}

void voice_assigner::note_off (unsigned const note) {
  for (auto &v : voices_) {
    if (v.note == note) {
      v.v.note_off ();
      v.note = unassigned;
    }
  }
}

void voice_assigner::set_wavetable (wavetable const *const w) {
  for (auto &v : voices_) {
    v.v.set_wavetable (w);
  }
}

void voice_assigner::set_envelope (envelope::phase const stage,
                                   double const value) {
  for (auto &v : voices_) {
    v.v.set_envelope (stage, value);
  }
}

double voice_assigner::tick () {
  return std::accumulate (std::begin (voices_), std::end (voices_), 0.0,
                          [] (double acc, vm &v) {
                            return acc + v.v.tick ().as_double ();
                          }) /
         voices_.size ();
}

}  // end namespace synth
