// -*- mode: c++; coding: utf-8-unix; -*-
#ifndef SYNTH_VOICE_HPP
#define SYNTH_VOICE_HPP

#include <array>

#include "envelope.hpp"
#include "nco.hpp"

namespace synth {

/// The formula connecting the MIDI note number and the base frequency, assuming
/// equal tuning based on A4=440Hz, is:
///
/// \f[
///   f = 440 \cdot 2^{(n - 69)/12}
/// \f]
///
/// \param note  A MIDI note number.
/// \returns  The frequency that corresponds to the MIDI note give by \p note.
inline double midi_note_to_frequency (unsigned const note) {
  return 440.0 * std::pow (2, (note - 69.0) / 12.0);
}

class voice {
public:
  using amplitude = wavetable::amplitude;
  using frequency = oscillator::frequency;

  explicit voice (wavetable const* const NONNULL w)
      : osc_{{oscillator{w}, oscillator{w}}} {}

  void note_on (unsigned const note);
  void note_off ();

  bool active () const { return env_.active (); }
  void set_wavetable (wavetable const* const NONNULL w);
  void set_envelope (envelope::phase stage, double value);

  amplitude tick ();

private:
  std::array<oscillator, 2> osc_;
  envelope env_;
};

}  // end namespace synth

#endif  // SYNTH_VOICE_HPP
