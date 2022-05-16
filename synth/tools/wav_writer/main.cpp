// -*- mode: c++; coding: utf-8-unix; -*-
// Standard library includes
#include <fstream>
#include <iostream>
#include <numeric>
#include <vector>

// synth library includes
#include "synth/envelope.hpp"
#include "synth/nco.hpp"
#include "synth/voice.hpp"
#include "synth/voice_assigner.hpp"

// Local includes
#include "wav_file.hpp"

using namespace synth;

namespace {

template <typename Traits>
void dump_wavetable (wavetable<Traits> const &w) {
  std::ostream &os = std::cout;
  os << std::hex;
  std::copy (std::begin (w), std::end (w),
             std::ostream_iterator<amplitude> (os, "\n"));
}

constexpr auto c4 = 60U;  // (middle C)
constexpr auto tuning = 440.0;
std::array<frequency, 8> const c_major{{
    frequency::fromfp (midi_note_to_frequency (tuning, c4)),        // C4
    frequency::fromfp (midi_note_to_frequency (tuning, c4 + 2U)),   // D4
    frequency::fromfp (midi_note_to_frequency (tuning, c4 + 4U)),   // E4
    frequency::fromfp (midi_note_to_frequency (tuning, c4 + 5U)),   // F4
    frequency::fromfp (midi_note_to_frequency (tuning, c4 + 7U)),   // G4
    frequency::fromfp (midi_note_to_frequency (tuning, c4 + 9U)),   // A4
    frequency::fromfp (midi_note_to_frequency (tuning, c4 + 11U)),  // B4
    frequency::fromfp (midi_note_to_frequency (tuning, c4 + 12U)),  // C5
}};

constexpr auto sample_rate = 48000U;
using oscillator_type = oscillator<sample_rate, nco_traits>;

class oscillator_double {
public:
  explicit constexpr oscillator_double (oscillator_type *const osc)
      : osc_{osc} {}
  double operator() () { return osc_->tick ().as_double (); }

private:
  oscillator_type *const osc_;
};

constexpr auto one_second = sample_rate;
constexpr auto quarter_second = sample_rate / size_t{4};

// A exponential chirp from 20Hz to 10kHz.
void chirp (std::vector<double> *const samples) {
  oscillator_type osc{&sine<nco_traits>};
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


int main () {
  std::vector<double> samples;

  if constexpr (/* DISABLES CODE */ (false)) {
    dump_wavetable (sine<nco_traits>);
  }

  if constexpr (/* DISABLES CODE */ (false)) {
    // Play a 440Hz tone for 2 seconds
    constexpr auto two_seconds = one_second * 2;
    oscillator_type osc{&sine<nco_traits>};
    osc.set_frequency (frequency::fromfp (440.0));
    samples.reserve (two_seconds);
    std::generate_n (std::back_inserter (samples), two_seconds,
                     oscillator_double{&osc});
  }

  if constexpr ((true)) {
    // Play a scale of C major using a collection of voices.
    synth::voice_assigner<sample_rate, nco_traits> voices;
    std::array<unsigned const, 8> const major_scale{
        {0U, 2U, 4U, 5U, 7U, 9U, 11U, 12U}};

    for (auto const note : major_scale) {
      voices.note_on (c4 + note);
      for (auto ctr = 0U; ctr < one_second / 4; ++ctr) {
        samples.emplace_back (voices.tick ());
      }
      // voices.note_off (note);
    }
    for (auto const note : major_scale) {
      voices.note_off (note);
    }
    for (auto ctr = 0U; ctr < one_second / 10U; ++ctr) {
      samples.emplace_back (voices.tick ());
    }
  }

  if constexpr (/* DISABLES CODE */ (false)) {
    // Play a scale of C major on a single oscillator
    oscillator_type osc{&sine<nco_traits>};
    for (auto const f : c_major) {
      osc.set_frequency (f);
      std::generate_n (std::back_inserter (samples), quarter_second,
                       oscillator_double{&osc});
    }
  }

  if constexpr (/* DISABLES CODE */ (false)) {
    // Play a scale of C major using two slightly detuned oscillators.
    oscillator_type osc1{&sine<nco_traits>};
    oscillator_type osc2{&sine<nco_traits>};
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
    oscillator_type osc{&sine<nco_traits>};
    osc.set_frequency (frequency::fromfp (440.0));
    osc.set_wavetable (&sawtooth<nco_traits>);
    std::generate_n (std::back_inserter (samples), two_seconds,
                     oscillator_double{&osc});
    osc.set_wavetable (&square<nco_traits>);
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
