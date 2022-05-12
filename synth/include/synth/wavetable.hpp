// -*- mode: c++; coding: utf-8-unix; -*-
#ifndef SYNTH_WAVETABLE_HPP
#define SYNTH_WAVETABLE_HPP

#include <algorithm>
#include <array>
#include <cmath>

#include "fixed.hpp"

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
constexpr inline double two_pi = 2.0 * pi;

using frequency = ufixed<32, 25>;  // 32-bit unsigned fixed, UQ25.7.

static_assert (std::is_same_v<frequency::value_type, uint32_t>);
static_assert (mask_v<frequency::integral_bits> >= 20U * 1000U,
               "Must be able to represent frequences up to 20kHz");

// TODO: we're currently storing ±(1+1/2^30) which we don't really need and
// wastes 2 of our precious bits. Instead, we should store just 32 fractional
// bits with values biased by 1.0 to eliminate negatives and scale to [0,2^32]
// to eliminate exact 1.0.
using amplitude = fixed<32, 1>;

struct osc_traits {
  // The number of entries in a wavetable is 2^N.
  static inline constexpr auto wavetable_N = 11U;

  /// Phase accumulation is performed in an M-bit integer register.
  static inline constexpr auto M = 32U;
  static_assert (M >= wavetable_N);

  static inline constexpr auto C_fractional_bits =
      M - frequency::fractional_bits - wavetable_N;

  /// When multiplying a UQa.b number by a UQc.d number, the result is
  /// UQ(a+c).(b+d). For the phase accumulator, a+c should be at least
  /// wavetable::N but may be more (we don't care if it overflows); b+d should
  /// be as large as possible to maintain precision.
  static inline constexpr auto accumulator_fractional_bits =
      frequency::fractional_bits + C_fractional_bits;

  using increment_type = ufixed<M, M - accumulator_fractional_bits>;
  static_assert (increment_type::total_bits == M);
  static_assert (increment_type::integral_bits == wavetable_N);
};

template <typename Traits>
class wavetable {
public:
  // The number of entries in the wavetable is 2^N.
  static constexpr inline auto N = Traits::wavetable_N;

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

template <typename Traits>
wavetable<Traits> const sine{
    [] (double const theta) { return std::sin (theta); }};

template <typename Traits>
wavetable<Traits> const square{
    [] (double const theta) { return theta <= pi ? 1.0 : -1.0; }};

template <typename Traits>
wavetable<Traits> const triangle{[] (double const theta) {
  return (theta <= pi ? theta : (two_pi - theta)) / half_pi - 1.0;
}};

template <typename Traits>
wavetable<Traits> const sawtooth{
    [] (double const theta) { return theta / pi - 1.0; }};

}  // end namespace synth

#endif  // SYNTH_WAVETABLE_HPP
