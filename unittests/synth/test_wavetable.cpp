#include <gmock/gmock.h>

#include "synth/wavetable.hpp"

using namespace synth;

TEST (Wavetable, Size) {
  auto const f = [] (double const theta) { return theta / pi - 1.0; };
  wavetable<nco_traits> wt{f};
  EXPECT_EQ (std::distance (std::begin (wt), std::end (wt)),
             1U << nco_traits::wavetable_N);
}

TEST (Wavetable, Entries) {
  auto const f = [] (double const theta) { return theta / pi - 1.0; };
  wavetable<nco_traits> wt{f};
  using phase_index_type =
      oscillator_info<decltype (wt)::traits>::phase_index_type;
  EXPECT_EQ (wt.phase_to_amplitude (phase_index_type::fromint (0)),
             amplitude::fromfp (f (0)));
}

/// Returns the population count (the number of bits that are 1), in an unsigned
/// value.
///
/// \tparam T  An unsigned integer type.
/// \param x  The value whose population count is to be returned.
/// \return  The population count of \p x.
template <typename T,
          typename = typename std::enable_if_t<std::is_unsigned_v<T>>>
constexpr unsigned pop_count (T const x) noexcept {
  return x == T{0} ? 0U : (x & T{1}) + pop_count (T{x >> 1U});
}

template <typename UFixed>
constexpr typename UFixed::value_type reciprocal (unsigned const f) {
  assert (pop_count (f) == 1U);
  return typename UFixed::value_type{1}
         << (UFixed::fractional_bits - pop_count (f - 1U));
}

struct my_traits {
  /// The number of entries in a wavetable is 2^wavetable_N.
  static constexpr auto wavetable_N = 2U;
  /// Phase accumulation is performed in an M-bit integer register.
  static constexpr auto M = 32U;
  static_assert (M >= wavetable_N);
};

TEST (Wavetable, f) {
  auto const f = [] (double const theta) { return theta / pi - 1.0; };
  wavetable<my_traits> wt{f};
  using phase_index_type =
      oscillator_info<decltype (wt)::traits>::phase_index_type;

  constexpr auto zero = phase_index_type::fromint (0);
  constexpr auto q0_75 = phase_index_type::fromint (
      0x0, 0b11 << (phase_index_type::fractional_bits - 2));  // 3/4=0.75
  constexpr auto q0_5 = phase_index_type::fromint (
      0x0, 1 << (phase_index_type::fractional_bits - 1));  // 1/2=0.5
  constexpr auto q0_25 = phase_index_type::fromint (
      0x0, 1 << (phase_index_type::fractional_bits - 2));  // 1/4=0.25
  constexpr auto q0_125 = phase_index_type::fromint (
      0x0, 1 << (phase_index_type::fractional_bits - 3));  // 0.125
  constexpr auto q0_0625 = phase_index_type::fromint (
      0x0, 1 << (phase_index_type::fractional_bits - 4));  // 0.0625

  EXPECT_EQ (phase_index_type::fromfp (3.0 / 4.0), q0_75);
  EXPECT_EQ (phase_index_type::fromfp (1.0 / 2.0), q0_5);
  EXPECT_EQ (phase_index_type::fromfp (1.0 / 4.0), q0_25);
  EXPECT_EQ (phase_index_type::fromfp (1.0 / 8.0), q0_125);
  EXPECT_EQ (phase_index_type::fromfp (1.0 / 16.0), q0_0625);

  EXPECT_EQ (wt.phase_to_amplitude (zero), amplitude::fromfp (-1.0));
  EXPECT_EQ (wt.phase_to_amplitude (q0_0625), amplitude::fromfp (-1.0));
  EXPECT_EQ (wt.phase_to_amplitude (q0_125), amplitude::fromfp (-1.0));
  EXPECT_EQ (wt.phase_to_amplitude (q0_25), amplitude::fromfp (-1.0));
  EXPECT_EQ (wt.phase_to_amplitude (q0_5), amplitude::fromfp (-1.0));

  //  EXPECT_EQ (wt.phase_to_amplitude(phase_index_type::fromfp (eighth)),
  //  amplitude::fromfp (-1.0)); EXPECT_EQ
  //  (wt.phase_to_amplitude(phase_index_type::fromfp (sixteenth)),
  //  amplitude::fromfp (-1.0));
}
