// -*- mode: c++; coding: utf-8-unix; -*-
#ifndef SYNTH_SC_NCO_HPP
#define SYNTH_SC_NCO_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#define SC_INCLUDE_FX 1
#include <systemc.h>

#include "scwavetable.hpp"

namespace scsynth {

class oscillator : public sc_core::sc_module {
public:
  static constexpr const auto sample_rate = 48000U;

  sc_in_clk clock;
  sc_in<bool> reset;
  sc_in<frequency> f;
  sc_out<amplitude> out;

private:
  sine_wavetable sine;
  sc_signal<sc_uint<sine_wavetable::N>> sample;
  sc_signal<amplitude> sample_out;

public:
  void tick () {
    std::cout << "oscillator tick\n";
    if (reset.read ()) {
      phase_ = 0;
    }
    increment_ =
        this->phase_increment (this->f.read ());  // TODO: only when f changes.

    sample = this->phase_accumulator ();
    // sine.phase.write (sample);
    //  sine.phase = sample;
    out.write (sine.out.read ());
    // out.write (w_->phase_to_amplitude (this->phase_accumulator ())); // TODO:
    // on clock edge.
  }

  //  void set_wavetable (wavetable const* const __nonnull w) { w_ = w; }
  //  void set_frequency (frequency const f) { increment_ =
  //  this->phase_increment (f); }

  // We trigger the below block with respect to positive
  // edge of the clock and also when ever reset changes state
  SC_CTOR (oscillator) : sine{"sine"} {
    std::cout << "Executing oscillator ctor\n";
    SC_METHOD (tick);
    sensitive << reset;
    sensitive << clock.pos ();  // positive edge.
    sensitive << f;
    sine.phase (sample);
    sine.out (sample_out);
  }

private:
  /// Phase accumulation is performed in an M-bit integer register.
  static constexpr auto M = 32U;
  static_assert (M >= sine_wavetable::N);

  // sine_wavetable const* __nonnull w_;
  sc_ufixed<M, sine_wavetable::N> increment_ = 0U;
  sc_ufixed<M, sine_wavetable::N> phase_ = 0U;

  /// phase_increment() wants to compute f/(S*r) where S is the sample rate and
  /// r is the number of entries in a wavetable. Everything but f is constant
  /// and we'd like to eliminate the division, so rearrange to get f*(r/S).
  /// Here, C gets the value r/S.
  static constexpr auto C2 =
      static_cast<double> (1U << sine_wavetable::N) / sample_rate;
  static_assert (C2 <= 1.0);
  static constexpr auto Cbits = M - frequency_fwl - sine_wavetable::N;
  static inline auto const C = sc_ufixed<Cbits, 0>{C2};

  /// When multiplying a UQa.b number by a UQc.d number, the result is
  /// UQ(a+c).(b+d). For the phase accumulator, a+c should be at least
  /// sine_wavetable::N but may be more (we don't care if it overflows); b+d
  /// should be as large as possible to maintain precision.
  static constexpr auto accumulator_fractional_bits = frequency_fwl + Cbits;

  sc_uint<sine_wavetable::N> phase_accumulator () {
    // The most significant (sine_wavetable::N) bits of the phase accumulator
    // output provide the index into the lookup table.
    auto const result = static_cast<sc_uint<sine_wavetable::N>> (phase_);
    phase_ += increment_;
    return result;
  }

  /// Computes the phase accumulator control value for frequency \p f.
  ///
  /// \param f  The frequency to be used expressed as a fixed-point number.
  /// \return The phase accumulator control value to be used to obtain
  ///   frequency \p f.
  static sc_ufixed<M, M - accumulator_fractional_bits> phase_increment (
      frequency const f) {
    return f * C;
  }
};

}  // namespace scsynth

#endif  // SYNTH_SC_NCO_HPP
