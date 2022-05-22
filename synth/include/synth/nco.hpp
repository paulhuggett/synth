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

template <unsigned SampleRate, typename Traits,
          typename Wavetable = wavetable<Traits>>
class oscillator {
public:
  using traits = Traits;
  static constexpr const auto sample_rate = SampleRate;

  constexpr oscillator () : w_{default_wavetable<Wavetable>{}()} {}
  constexpr explicit oscillator (Wavetable const* const NONNULL w) : w_{w} {}

  void set_wavetable (Wavetable const* const NONNULL w) { w_ = w; }
  void set_frequency (frequency const f) {
    increment_ = oscillator::phase_increment (f);
  }

  amplitude tick () {
    return w_->phase_to_amplitude (this->phase_accumulator ());
  }

private:
  static_assert (traits::wavetable_N == Wavetable::traits::wavetable_N,
                 "The wavetable traits and oscillator traits must match");
  static_assert (traits::M == Wavetable::traits::M,
                 "The wavetable traits and oscillator traits must match");

  /// phase_increment() wants to compute f/(S*r) where S is the sample rate and
  /// r is the number of entries in a wavetable. Everything but f is constant
  /// and we'd like to eliminate the division, so rearrange to get f*(r/S).
  static constexpr auto C =
      ufixed<oscillator_info<traits>::C_fractional_bits, 0>::fromfp (
          static_cast<double> (1U << traits::wavetable_N) / sample_rate);
  static_assert (C.get () > 0U,
                 "There are insufficient fractional bits for the phase "
                 "accumulator constant");

  using phase_index_type = typename oscillator_info<traits>::phase_index_type;

  Wavetable const* NONNULL w_;
  phase_index_type increment_;
  phase_index_type phase_;

  phase_index_type phase_accumulator () {
    auto const result = phase_;
    phase_ =
        phase_index_type{static_cast<typename phase_index_type::value_type> (
            phase_.get () + increment_.get ())};
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
    return phase_index_type{static_cast<typename phase_index_type::value_type> (
        f.get () * C.get ())};
  }
};

}  // end namespace synth

#endif  // SYNTH_NCO_HPP
