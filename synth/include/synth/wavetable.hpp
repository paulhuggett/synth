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

class wavetable {
public:
  // The number of entries in the wavetable is 2^N.
  static constexpr inline auto N = 11U;

  // TODO: we're currently storing ±(1+1/2^30) which we don't really need and
  // wastes 2 of our precious bits. Instead, we should store just 32 fractional
  // bits with values biased by 1.0 to eliminate negatives and scale to [0,2^32]
  // to eliminate exact 1.0.
  using amplitude = fixed<1U, 30U>;

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

}  // end namespace synth

#endif  // SYNTH_WAVETABLE_HPP
