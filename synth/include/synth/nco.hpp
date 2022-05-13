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
      ufixed<oscillator_info<Traits>::C_fractional_bits, 0>::fromfp (
          static_cast<double> (1U << Traits::wavetable_N) / sample_rate);

  using phase_index_type = typename oscillator_info<Traits>::phase_index_type;

  wavetable<Traits> const* NONNULL w_;
  phase_index_type increment_;
  phase_index_type phase_;

  constexpr phase_index_type phase_accumulator () {
    auto const result = phase_;
    phase_ = phase_index_type{phase_.get () + increment_.get ()};
    return result;
  }

  /// Computes the phase accumulator control value for frequency \p f.
  ///
  /// \param f  The frequency to be used expressed as a fixed-point number.
  /// \return The phase accumulator control value to be used to obtain
  ///   frequency \p f.
  static constexpr phase_index_type phase_increment (frequency const f) {
    // '>=' here because we don't care if f+C overflows.
    static_assert (decltype (f)::integral_bits + decltype (C)::integral_bits >=
                   phase_index_type::integral_bits);
    static_assert (decltype (f)::fractional_bits +
                       decltype (C)::fractional_bits ==
                   phase_index_type::fractional_bits);
    return phase_index_type{f.get () * C.get ()};
  }
};

}  // end namespace synth

#endif  // SYNTH_NCO_HPP
