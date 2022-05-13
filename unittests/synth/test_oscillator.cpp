#include <gmock/gmock.h>

#include <vector>

#include "synth/nco.hpp"

using namespace synth;

namespace {

class Oscillator : public testing::Test {
public:
  static constexpr auto sample_rate = 16384U;  // Must be a power of two.

  wavetable<nco_traits> wt_{sawtooth<nco_traits>};
  oscillator<sample_rate, nco_traits> osc_{&wt_};

  std::vector<amplitude> make_samples (unsigned cycle_length);
};

auto Oscillator::make_samples (unsigned cycle_length)
    -> std::vector<amplitude> {
  assert (sample_rate % cycle_length == 0U);
  osc_.set_frequency (frequency::fromint (sample_rate / cycle_length));

  std::vector<amplitude> samples;
  ++cycle_length;  // One extra sample to catch the cycle repeat start.
  samples.reserve (cycle_length);
  std::generate_n (std::back_inserter (samples), cycle_length,
                   [this] () { return osc_.tick (); });
  return samples;
}

}  // end anonymous namespace

TEST_F (Oscillator, Nyquist) {
  EXPECT_THAT (
      make_samples (2U),  // Nyquist frequency
      testing::ElementsAre (amplitude::fromfp (-1.0), amplitude::fromfp (0.0),
                            amplitude::fromfp (-1.0)));
}

TEST_F (Oscillator, HalfNyquist) {
  EXPECT_THAT (
      make_samples (4U),  // Nyquist / 2
      testing::ElementsAre (amplitude::fromfp (-1.0), amplitude::fromfp (-0.5),
                            amplitude::fromfp (0.0), amplitude::fromfp (0.5),
                            amplitude::fromfp (-1.0)));
}

TEST_F (Oscillator, QuarterNyquist) {
  EXPECT_THAT (
      make_samples (8U),  // Nyquist / 4
      testing::ElementsAre (amplitude::fromfp (-1.0), amplitude::fromfp (-0.75),
                            amplitude::fromfp (-0.5), amplitude::fromfp (-0.25),
                            amplitude::fromfp (0.0), amplitude::fromfp (0.25),
                            amplitude::fromfp (0.5), amplitude::fromfp (0.75),
                            amplitude::fromfp (-1.0)));
}
