// -*- mode: c++; coding: utf-8-unix; -*-
#ifndef SYNTH_NCO_HPP
#define SYNTH_NCO_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <limits>

#include "fixed.hpp"
#include "uint.hpp"
#include "wavetable.hpp"

#if !defined(__clang_major__) || __clang_major__ < 7
#define NONNULL
#else
#define NONNULL _Nonnull
#endif

namespace synth {

class oscillator {
public:
  using amplitude = wavetable::amplitude;
  using frequency = ufixed<32, 25>;  // 32-bit unsigned fixed, UQ25.7.
  static_assert (std::is_same_v<frequency::value_type, uint32_t>);
  static_assert (mask_v<frequency::integral_bits> >= 20U * 1000U,
                 "Must be able to represent frequences up to 20kHz");

  static inline constexpr const auto sample_rate = 48000U;

  constexpr explicit oscillator (wavetable const* const NONNULL w) : w_{w} {}

  void set_wavetable (wavetable const* const NONNULL w) { w_ = w; }
  void set_frequency (frequency const f) {
    increment_ = oscillator::phase_increment (f);
  }

  constexpr amplitude tick () {
    return w_->phase_to_amplitude (this->phase_accumulator ());
  }

private:
  /// Phase accumulation is performed in an M-bit integer register.
  static inline constexpr auto M = 32U;
  static_assert (M >= wavetable::N);

  wavetable const* NONNULL w_;
  uinteger_t<M> increment_ = 0U;
  uinteger_t<M> phase_ = 0U;

  /// phase_increment() wants to compute f/(S*r) where S is the sample rate and
  /// r is the number of entries in a wavetable. Everything but f is constant
  /// and we'd like to eliminate the division, so rearrange to get f*(r/S).
  static inline constexpr auto C =
      ufixed<M - frequency::fractional_bits - wavetable::N, 0>::fromfp (
          static_cast<double> (1U << wavetable::N) / sample_rate);

  /// When multiplying a UQa.b number by a UQc.d number, the result is
  /// UQ(a+c).(b+d). For the phase accumulator, a+c should be at least
  /// wavetable::N but may be more (we don't care if it overflows); b+d should
  /// be as large as possible to maintain precision.
  static inline constexpr auto accumulator_fractional_bits =
      frequency::fractional_bits + decltype (C)::fractional_bits;

  constexpr uinteger_t<wavetable::N> phase_accumulator () {
    // The most significant (wavetable::N) bits of the phase accumulator output
    // provide the index into the lookup table.
    auto const result = static_cast<uinteger_t<wavetable::N>> (
        (phase_ >> accumulator_fractional_bits) & mask_v<wavetable::N>);
    phase_ += increment_;
    return result;
  }

  /// Computes the phase accumulator control value for frequency \p f.
  ///
  /// \param f  The frequency to be used expressed as a fixed-point number.
  /// \return The phase accumulator control value to be used to obtain
  ///   frequency \p f.
  static constexpr uinteger_t<M> phase_increment (frequency const f) {
    using incr_type = ufixed<M, M - accumulator_fractional_bits>;
    static_assert (incr_type::total_bits == M);
    static_assert (incr_type::integral_bits == wavetable::N);

    return incr_type{f.get () * C.get ()}.get ();
  }
};

}  // end namespace synth

#endif  // SYNTH_NCO_HPP
