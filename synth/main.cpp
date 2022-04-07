// -*- mode: c++; coding: utf-8-unix; -*-
#include <fstream>
#include <iostream>
#include <numeric>
#include <vector>

#include "envelope.hpp"
#include "nco.hpp"
#include "voice.hpp"
#include "wav_file.hpp"

using namespace synth;

namespace {

// theta is from [0..2π)
wavetable const sine{[] (double const theta) { return std::sin (theta); }};
wavetable const saw{
    [] (double const theta) { return (2.0 / two_pi) * theta - 1.0; }};
wavetable const triangle{[] (double const theta) {
  return (theta <= pi ? theta : (two_pi - theta)) / half_pi - 1.0;
}};
wavetable const square{
    [] (double const theta) { return theta <= pi ? 1.0 : -1.0; }};

void dump_wavetable (wavetable const &w) {
  std::ostream &os = std::cout;
  os << std::hex;
  std::copy (std::begin (w), std::end (w),
             std::ostream_iterator<wavetable::amplitude> (os, "\n"));
}

using frequency = oscillator::frequency;

constexpr auto c4 = 60U;  // (middle C)
std::array<frequency, 8> const c_major{{
    frequency::fromfp (midi_note_to_frequency (c4)),        // C4
    frequency::fromfp (midi_note_to_frequency (c4 + 2U)),   // D4
    frequency::fromfp (midi_note_to_frequency (c4 + 4U)),   // E4
    frequency::fromfp (midi_note_to_frequency (c4 + 5U)),   // F4
    frequency::fromfp (midi_note_to_frequency (c4 + 7U)),   // G4
    frequency::fromfp (midi_note_to_frequency (c4 + 9U)),   // A4
    frequency::fromfp (midi_note_to_frequency (c4 + 11U)),  // B4
    frequency::fromfp (midi_note_to_frequency (c4 + 12U)),  // C5
}};

class oscillator_double {
public:
  explicit constexpr oscillator_double (oscillator *const osc) : osc_{osc} {}
  double operator() () { return osc_->tick ().as_double (); }

private:
  oscillator *const osc_;
};

constexpr auto sample_rate = oscillator::sample_rate;
constexpr auto one_second = sample_rate;
constexpr auto quarter_second = sample_rate / size_t{4};

// A exponential chirp from 20Hz to 10kHz.
void chirp (std::vector<double> *const samples) {
  oscillator osc{&sine};
  oscillator_double oscd{&osc};

  constexpr auto duration = one_second * 2;
  constexpr auto growth_rate = 1.0 + .125e-3;
  constexpr auto maxf = 10e3;  // The max sweep frequency.
  constexpr auto k = maxf / (duration * growth_rate);
  for (auto ctr = 0U; ctr < duration; ++ctr) {
    auto f = k * std::pow (ctr, growth_rate);
    osc.set_frequency (frequency::fromfp (f));
    samples->emplace_back (oscd ());
  }
}

}  // end anonymous namespace

class voice_assign {
public:
  voice_assign () = default;

  void note_on (unsigned const note) {
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

  void note_off (unsigned const note) {
    for (auto &v : voices_) {
      if (v.note == note) {
        v.v.note_off ();
        v.note = unassigned;
      }
    }
  }

  double tick () {
    return std::accumulate (std::begin (voices_), std::end (voices_), 0.0,
                            [] (double acc, vm &v) {
                              return acc + v.v.tick ().as_double ();
                            }) /
           voices_.size ();
  }

private:
  static constexpr auto unassigned = std::numeric_limits<unsigned>::max ();
  struct vm {
    vm () : v{&sine}, note{unassigned} {}
    voice v;
    unsigned note;
  };
  std::array<vm, 4> voices_;
  unsigned next_ = 0U;
};

int main () {
  std::vector<double> samples;

  if constexpr (/* DISABLES CODE */ (false)) {
    dump_wavetable (sine);
  }

  if constexpr (/* DISABLES CODE */ (false)) {
    // Play a 440Hz tone for 2 seconds
    constexpr auto two_seconds = one_second * 2;
    oscillator osc{&sine};
    osc.set_frequency (frequency::fromfp (440.0));
    samples.reserve (two_seconds);
    std::generate_n (std::back_inserter (samples), two_seconds,
                     oscillator_double{&osc});
  }

  if constexpr ((true)) {
    // Play a scale of C major using a collection of voices.
    voice_assign voices;
    std::array<unsigned, 8> const c_major_scale{{
        c4,        // C4
        c4 + 2U,   // D4
        c4 + 4U,   // E4
        c4 + 5U,   // F4
        c4 + 7U,   // G4
        c4 + 9U,   // A4
        c4 + 11U,  // B4
        c4 + 12U,  // C5
    }};

    for (auto const note : c_major_scale) {
      voices.note_on (note);
      for (auto ctr = 0U; ctr < one_second / 4; ++ctr) {
        samples.emplace_back (voices.tick ());
      }
      // voices.note_off (note);
    }
    for (auto const note : c_major_scale) {
      voices.note_off (note);
    }
    for (auto ctr = 0U; ctr < one_second / 10U; ++ctr) {
      samples.emplace_back (voices.tick ());
    }
  }

  if constexpr (/* DISABLES CODE */ (false)) {
    // Play a scale of C major on a single oscillator
    oscillator osc{&sine};
    for (auto const f : c_major) {
      osc.set_frequency (f);
      std::generate_n (std::back_inserter (samples), quarter_second,
                       oscillator_double{&osc});
    }
  }

  if constexpr (/* DISABLES CODE */ (false)) {
    // Play a scale of C major using two slightly detuned oscillators.
    oscillator osc1{&sine};
    oscillator osc2{&sine};
    for (auto const f : c_major) {
      static constexpr auto detune = frequency::fromfp (4.0);
      osc1.set_frequency (f);
      osc2.set_frequency (f + detune);
      std::generate_n (std::back_inserter (samples), quarter_second, [&] {
        return (osc1.tick ().as_double () + osc2.tick ().as_double ()) / 2.0;
      });
    }
  }

  if constexpr (/* DISABLES CODE */ (false)) {
    static constexpr auto two_seconds = sample_rate * 2U;
    oscillator osc{&sine};
    osc.set_frequency (frequency::fromfp (440.0));
    osc.set_wavetable (&saw);
    std::generate_n (std::back_inserter (samples), two_seconds,
                     oscillator_double{&osc});
    osc.set_wavetable (&square);
    std::generate_n (std::back_inserter (samples), two_seconds,
                     oscillator_double{&osc});
  }

  if constexpr (/* DISABLES CODE */ (false)) {
    chirp (&samples);
  }

  std::vector<uint8_t> wave_file;
  emit_wave_file (std::begin (samples), std::end (samples), sample_rate,
                  std::back_inserter (wave_file));
  std::ofstream of{"./output.wav", std::ios::binary};
  of.write (reinterpret_cast<char *> (wave_file.data ()),
            static_cast<std::streamsize> (wave_file.size ()));
}
