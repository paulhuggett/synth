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
