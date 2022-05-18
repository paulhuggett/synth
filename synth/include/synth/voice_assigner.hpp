// -*- mode: c++; coding: utf-8-unix; -*-
#ifndef SYNTH_VOICE_ASSIGNER_HPP
#define SYNTH_VOICE_ASSIGNER_HPP

#include <array>
#include <limits>
#include <numeric>

#include "synth/voice.hpp"

namespace synth {

template <unsigned SampleRate, typename Traits>
class voice_assigner {
public:
  voice_assigner () = default;

  void note_on (unsigned note);
  void note_off (unsigned note);

  double tick ();

  void set_wavetable (wavetable<Traits> const *w);
  void set_envelope (typename envelope<SampleRate>::phase stage, double value);

  uint16_t active_voices () const;

private:
  static constexpr auto unassigned = std::numeric_limits<unsigned>::max ();
  struct vm {
    voice<SampleRate, Traits> v{&triangle<Traits>};
    unsigned note = unassigned;
  };
  std::array<vm, 8> voices_;
  unsigned next_ = 0U;
};

// note on
// ~~~~~~~
template <unsigned SampleRate, typename Traits>
void voice_assigner<SampleRate, Traits>::note_on (unsigned const note) {
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

// note off
// ~~~~~~~~
template <unsigned SampleRate, typename Traits>
void voice_assigner<SampleRate, Traits>::note_off (unsigned const note) {
  for (auto &v : voices_) {
    if (v.note == note) {
      v.v.note_off ();
      v.note = unassigned;
    }
  }
}

// active voices
// ~~~~~~~~~~~~~
template <unsigned SampleRate, typename Traits>
uint16_t voice_assigner<SampleRate, Traits>::active_voices () const {
  auto result = uint16_t{0};
  auto count = 0U;
  for (vm const &v : voices_) {
    result |= static_cast<uint16_t> (v.v.active ()) << count++;
  }
  return result;
}

// set wavetable
// ~~~~~~~~~~~~~
template <unsigned SampleRate, typename Traits>
void voice_assigner<SampleRate, Traits>::set_wavetable (
    wavetable<Traits> const *const w) {
  for (auto &v : voices_) {
    v.v.set_wavetable (w);
  }
}

// set envelope
// ~~~~~~~~~~~~
template <unsigned SampleRate, typename Traits>
void voice_assigner<SampleRate, Traits>::set_envelope (
    typename envelope<SampleRate>::phase const stage, double const value) {
  for (auto &v : voices_) {
    v.v.set_envelope (stage, value);
  }
}

// tick
// ~~~~
template <unsigned SampleRate, typename Traits>
double voice_assigner<SampleRate, Traits>::tick () {
  return std::accumulate (std::begin (voices_), std::end (voices_), 0.0,
                          [] (double acc, vm &v) {
                            return acc + v.v.tick ().as_double ();
                          }) /
         voices_.size ();
}

}  // end namespace synth

#endif  // SYNTH_VOICE_ASSIGNER_HPP
