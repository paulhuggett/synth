// -*- mode: c++; coding: utf-8-unix; -*-
#ifndef SYNTH_VOICE_HPP
#define SYNTH_VOICE_HPP

#include <array>

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

  explicit constexpr voice (wavetable const* const NONNULL w)
      : osc_{{oscillator{w}, oscillator{w}}} {}

  void note_on (unsigned const note) {
    auto const f = midi_note_to_frequency (note);
    osc_[0].set_frequency (frequency::fromfp (f));        // 8'
    osc_[1].set_frequency (frequency::fromfp (f / 2.0));  // 16'
    env_.note_on ();
  }
  void note_off () { env_.note_off (); }

  amplitude tick () {
    double const a = std::accumulate (std::begin (osc_), std::end (osc_), 0.0,
                                      [] (double const acc, oscillator& osc) {
                                        return acc + osc.tick ().as_double ();
                                      });
    return env_.tick (amplitude::fromfp (a));
  }

private:
  std::array<oscillator, 2> osc_;
  synth::envelope env_;
};

}  // end namespace synth

#endif  // SYNTH_VOICE_HPP
