// -*- mode: c++; coding: utf-8-unix; -*-
// Standard library includes
#include <chrono>
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
#include "poly_blep_sawtooth.hpp"
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

constexpr auto sample_rate = 96000U;
using oscillator_type = oscillator<sample_rate, nco_traits>;

template <typename Oscillator>
class oscillator_double {
public:
  explicit constexpr oscillator_double (Oscillator *NONNULL const osc)
      : osc_{osc} {}
  void set_frequency (frequency const f) { osc_->set_frequency (f); }
  double operator() () { return osc_->tick ().as_double (); }

private:
  Oscillator *NONNULL osc_;
};
oscillator_double ()->oscillator_double<oscillator_type>;

/// Generates an exponential chirp.
///
/// \param osc  The oscillator to be used to generate output.
/// \param minf  The minimum frequency of the chirp (Hz).
/// \param maxf  The maximum frequency of the chirp (Hz).
/// \param time  The duration of the chirp.
/// \param out  A LegacyOutputIterator iterator to which samples will be written.
/// \returns  Output iterator to the element in the destination range, one past the last element written.
template <typename Oscillator, typename OutputIterator, typename DurationRep,
          typename DurationPeriod>
OutputIterator chirp (Oscillator *NONNULL osc, double minf, double maxf,
                      std::chrono::duration<DurationRep, DurationPeriod> time,
                      OutputIterator out) {
  oscillator_double<Oscillator> oscd{osc};

  auto const time_seconds = std::chrono::duration<double>{time}.count ();
  auto const samples = static_cast<unsigned long> (
      std::round (time_seconds * Oscillator::sample_rate));
  auto const n = (maxf - minf) / (std::pow (2.0, time_seconds) - 1);
  for (auto t = 0U; t < samples; ++t) {
    auto const f =
        n * std::pow (2.0, static_cast<double> (t) / Oscillator::sample_rate) -
        n + minf;
    oscd.set_frequency (frequency::fromfp (f));
    *(out++) = oscd ();
  }
  return out;
}

constexpr auto one_second = sample_rate;
constexpr auto quarter_second = sample_rate / size_t{4};

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

  if constexpr (/* DISABLES CODE */ (false)) {
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
      osc2.set_frequency ((f + detune).cast<frequency> ());
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

  if constexpr (/* DISABLES CODE */ (true)) {
    poly_blep_sawtooth_oscillator<sample_rate> osc;
    chirp (&osc, 0.0, 10000.0, std::chrono::seconds{10},
           std::back_inserter (samples));
  }

  std::vector<uint8_t> wave_file;
  emit_wave_file (std::begin (samples), std::end (samples), sample_rate,
                  std::back_inserter (wave_file));
  std::ofstream of{"./output.wav", std::ios::binary};
  of.write (reinterpret_cast<char *> (wave_file.data ()),
            static_cast<std::streamsize> (wave_file.size ()));
}
