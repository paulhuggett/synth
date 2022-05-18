// -*- mode: c++; coding: utf-8-unix; -*-
#ifndef SYNTH_SCWAVETABLE_HPP
#define SYNTH_SCWAVETABLE_HPP

#include <algorithm>
#include <array>
#include <cmath>

#define SC_INCLUDE_FX 1
#include <systemc.h>

namespace scsynth {

#ifdef M_PI
constexpr inline double pi = M_PI;
#else
constexpr inline double pi = 3.14159265358979323846264338327950288;
#endif
#ifdef M_PI_2
constexpr inline double half_pi = M_PI_2;
#else
constexpr inline double half_pi = pi / 2.0;
#endif
constexpr inline double two_pi = 2.0 * pi;

// TODO: we're currently storing ±(1+1/2^30) which we don't really need and
// wastes 2 of our precious bits. Instead, we should store just 32 fractional
// bits with values biased by 1.0 to eliminate negatives and scale to [0,2^32]
// to eliminate exact 1.0.
using amplitude = sc_fixed<32, 1>;

constexpr auto frequency_wl = 32;
constexpr auto frequency_iwl = 25;
constexpr auto frequency_fwl = frequency_wl - frequency_iwl;
using frequency = sc_ufixed<frequency_wl, frequency_iwl>;

class sine_wavetable : public sc_module {
public:
  // The number of entries in the wavetable is 2^N.
  static constexpr auto N = 11U;

  //  sc_in_clk clock;
  sc_in<sc_uint<N>> phase;
  sc_out<amplitude> out;

  SC_CTOR (sine_wavetable) {
    // Initialize the lookup table with exactly one cycle of our waveform.
    auto k = size_t{0};
    auto f = [] (double const theta) { return std::sin (theta); };
    std::generate (std::begin (y_), std::end (y_),
                   [f, &k] { return f (two_pi * k++ / table_size_); });

    std::cout << "Executing sine_wavetable ctor\n";
    SC_METHOD (phase_to_amplitude);
    // sensitive << clock.pos(); // positive edge.
    sensitive << phase;
  }

  void phase_to_amplitude () {  // amplitude phase_to_amplitude (sc_uint<N>
                                // const phase) const noexcept {
    std::cout << "sine_wavetable::phase_to_amplitude\n";
    auto const p = phase.read ();
    assert (p < table_size_);
    out.write (y_[p]);
  }

  constexpr auto begin () const { return std::begin (y_); }
  constexpr auto end () const { return std::end (y_); }

private:
  static constexpr auto table_size_ = size_t{1} << N;
  std::array<amplitude, table_size_> y_;
};

}  // namespace scsynth

#endif  // SYNTH_SCWAVETABLE_HPP
