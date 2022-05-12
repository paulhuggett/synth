// -*- mode: c++; coding: utf-8-unix; -*-
#ifndef SYNTH_VOICE_HPP
#define SYNTH_VOICE_HPP

#include <array>
#include <numeric>

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

template <unsigned SampleRate, typename Traits>
class voice {
  using oscillator_type = oscillator<SampleRate, Traits>;

public:
  explicit voice (wavetable<Traits> const* const NONNULL w)
      : osc_{{oscillator_type{w}, oscillator_type{w}}} {}

  void note_on (unsigned const note);
  void note_off ();

  bool active () const { return env_.active (); }
  void set_wavetable (wavetable<Traits> const* const NONNULL w);
  void set_envelope (typename envelope<SampleRate>::phase stage, double value);

  amplitude tick ();

private:
  std::array<oscillator_type, 2> osc_;
  envelope<SampleRate> env_;

  static inline double saturate (double const a) {
    return std::min (std::max (a, -1.0), 1.0);
  }
};

// note on
// ~~~~~~~
template <unsigned SampleRate, typename Traits>
void voice<SampleRate, Traits>::note_on (unsigned const note) {
  auto const f = midi_note_to_frequency (note);
  osc_[0].set_frequency (frequency::fromfp (f));  // 8'
  osc_[1].set_frequency (frequency::fromfp (f + 4.0));
  env_.note_on ();
}

// note off
// ~~~~~~~~
template <unsigned SampleRate, typename Traits>
void voice<SampleRate, Traits>::note_off () {
  env_.note_off ();
}

// set wavetable
// ~~~~~~~~~~~~~
template <unsigned SampleRate, typename Traits>
void voice<SampleRate, Traits>::set_wavetable (
    wavetable<Traits> const* const NONNULL w) {
  for (auto& osc : osc_) {
    osc.set_wavetable (w);
  }
}

// set envelope
// ~~~~~~~~~~~~
template <unsigned SampleRate, typename Traits>
void voice<SampleRate, Traits>::set_envelope (
    typename envelope<SampleRate>::phase const stage, double const value) {
  env_.set (stage, value);
}

// tick
// ~~~~
template <unsigned SampleRate, typename Traits>
auto voice<SampleRate, Traits>::tick () -> amplitude {
  if (!env_.active ()) {
    return amplitude::fromfp (0.0);
  }
  // Mix the output from the oscillators.
  double const a =
      std::accumulate (std::begin (osc_), std::end (osc_), 0.0,
                       [] (double const acc, oscillator_type& osc) {
                         return saturate (acc + osc.tick ().as_double ());
                       });
  return env_.tick (amplitude::fromfp (a));
}

}  // end namespace synth

#endif  // SYNTH_VOICE_HPP
