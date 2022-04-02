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

namespace synth {

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
constexpr inline double two_pi = 2.0 * M_PI;

// TODO: we're currently storing ±(1+1/2^30) which we don't really need and
// wastes 2 of our precious bits. Instead, we should store just 32 fractional
// bits with values biased by 1.0 to eliminate negatives and scale to [0,2^32]
// to eliminate exact 1.0.
using amplitude = fixed<1U, 30U>;

class wavetable {
public:
  // The number of entries in the wavetable is 2^N.
  static constexpr inline auto N = 11U;

  /// \tparam Function  A function with signature equivalent to double(double).
  /// \param f A function f(θ) which will be invoked with θ from [0..2π).
  template <typename Function>
  explicit wavetable (Function f) {
    // Initialize the lookup table with exactly one cycle of our waveform.
    auto k = size_t{0};
    std::generate (std::begin (y_), std::end (y_), [f, &k] {
      return amplitude::fromfp (f (two_pi * k++ / table_size_));
    });
  }

  constexpr amplitude phase_to_amplitude (
      uinteger_t<N> const phase) const noexcept {
    assert (phase < table_size_);
    return y_[phase];
  }

  constexpr auto begin () const { return std::begin (y_); }
  constexpr auto end () const { return std::end (y_); }

private:
  static constexpr auto table_size_ = size_t{1} << N;
  std::array<amplitude, table_size_> y_;
};

class oscillator {
public:
  using frequency = ufixed<25U, 7U>;
  static_assert (std::is_same_v<frequency::value_type, uint32_t>);
  static_assert (mask_v<frequency::integral_bits> >= 20U * 1000U,
                 "Must be able to represent frequences up to 20kHz");

  static inline constexpr const auto sample_rate = 48000U;

  constexpr explicit oscillator (wavetable const* const __nonnull w) : w_{w} {}

  void set_wavetable (wavetable const* const __nonnull w) { w_ = w; }
  void set_frequency (frequency const f) {
    increment_ = this->phase_increment (f);
  }

  constexpr amplitude tick () {
    return w_->phase_to_amplitude (this->phase_accumulator ());
  }

private:
  // Phase accumulation is performed in an M-bit integer register.
  static inline constexpr auto M = 32U;
  static_assert (M >= wavetable::N);

  wavetable const* __nonnull w_;
  uinteger_t<M> increment_ = 0U;
  uinteger_t<M> phase_ = 0U;

  // phase_increment() wants to compute f/(S*r) where S is the sample rate and r
  // is the number of entries in a wavetable. Everything but f is constant and
  // we'd like to eliminate the division, so rearrange to get f*(r/S).
  static inline constexpr auto C =
      ufixed<0, M - frequency::fractional_bits - wavetable::N>::fromfp (
          static_cast<double> (1U << wavetable::N) / sample_rate);

  // When multiplying a UQa.b number by a UQc.d number, the result is
  // UQ(a+c).(b+d). For the phase accumulator, a+c should be at least
  // wavetable::N but may be more (we don't care if it overflows); b+d should be
  // as large as possible to maintain precision.
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

  /// Returns the phase accumulator control value to be used to obtain frequency
  /// \p f. \param f  The frequency to be used expressed as a fixed-point number
  static constexpr uinteger_t<M> phase_increment (frequency const f) {
    auto const r =
        ufixed<M - accumulator_fractional_bits, accumulator_fractional_bits>{
            f.get () * C.get ()};
    static_assert (decltype (r)::total_bits == M);
    static_assert (decltype (r)::integral_bits == wavetable::N);
    return r.get ();
  }
};

}  // end namespace synth

#endif  // SYNTH_NCO_HPP
