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

template <unsigned SampleRate, typename Traits>
class oscillator {
public:
  static inline constexpr const auto sample_rate = SampleRate;

  constexpr explicit oscillator (wavetable<Traits> const* const NONNULL w)
      : w_{w} {}

  void set_wavetable (wavetable<Traits> const* const NONNULL w) { w_ = w; }
  void set_frequency (frequency const f) {
    increment_ = oscillator::phase_increment (f);
  }

  constexpr amplitude tick () {
    return w_->phase_to_amplitude (this->phase_accumulator ());
  }

private:
  /// phase_increment() wants to compute f/(S*r) where S is the sample rate and
  /// r is the number of entries in a wavetable. Everything but f is constant
  /// and we'd like to eliminate the division, so rearrange to get f*(r/S).
  static inline constexpr auto C =
      ufixed<Traits::C_fractional_bits, 0>::fromfp (
          static_cast<double> (1U << Traits::wavetable_N) / sample_rate);

  using increment_type = typename Traits::increment_type;

  wavetable<Traits> const* NONNULL w_;
  increment_type increment_;
  uinteger_t<Traits::M> phase_ = 0U;

  constexpr uinteger_t<Traits::wavetable_N> phase_accumulator () {
    // The most significant (wavetable::N) bits of the phase accumulator output
    // provide the index into the lookup table.
    auto const result = static_cast<uinteger_t<Traits::wavetable_N>> (
        (phase_ >> Traits::accumulator_fractional_bits) &
        mask_v<Traits::wavetable_N>);
    phase_ += increment_.get ();
    return result;
  }

  /// Computes the phase accumulator control value for frequency \p f.
  ///
  /// \param f  The frequency to be used expressed as a fixed-point number.
  /// \return The phase accumulator control value to be used to obtain
  ///   frequency \p f.
  static constexpr increment_type phase_increment (frequency const f) {
    // '>=' here because we don't care if f+C overflows.
    static_assert (decltype (f)::integral_bits + decltype (C)::integral_bits >=
                   increment_type::integral_bits);
    static_assert (decltype (f)::fractional_bits +
                       decltype (C)::fractional_bits ==
                   increment_type::fractional_bits);
    return increment_type{f.get () * C.get ()};
  }
};

}  // end namespace synth

#endif  // SYNTH_NCO_HPP
