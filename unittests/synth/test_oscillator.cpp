#include <gmock/gmock.h>

#include <vector>

#include "synth/nco.hpp"

namespace {

class Oscillator : public testing::Test {
public:
  static constexpr auto sample_rate = 16384U;  // Must be a power of two.
  using oscillator = synth::basic_oscillator<sample_rate>;

  synth::wavetable wt_{synth::sawtooth};
  oscillator osc_{&wt_};

  std::vector<oscillator::amplitude> make_samples (unsigned cycle_length);
};

auto Oscillator::make_samples (unsigned cycle_length)
    -> std::vector<oscillator::amplitude> {
  osc_.set_frequency (
      oscillator::frequency::fromint (sample_rate / cycle_length));

  std::vector<oscillator::amplitude> samples;
  ++cycle_length;  // One extra sample to catch the cycle repeat start.
  samples.reserve (cycle_length);
  std::generate_n (std::back_inserter (samples), cycle_length,
                   [this] () { return osc_.tick (); });
  return samples;
}

}  // end anonymous namespace

TEST_F (Oscillator, Nyquist) {
  EXPECT_THAT (make_samples (2U),  // Nyquist frequency
               testing::ElementsAre (oscillator::amplitude::fromfp (-1.0),
                                     oscillator::amplitude::fromfp (0.0),
                                     oscillator::amplitude::fromfp (-1.0)));
}

TEST_F (Oscillator, HalfNyquist) {
  EXPECT_THAT (make_samples (4U),  // Nyquist / 2
               testing::ElementsAre (oscillator::amplitude::fromfp (-1.0),
                                     oscillator::amplitude::fromfp (-0.5),
                                     oscillator::amplitude::fromfp (0.0),
                                     oscillator::amplitude::fromfp (0.5),
                                     oscillator::amplitude::fromfp (-1.0)));
}

TEST_F (Oscillator, QuarterNyquist) {
  EXPECT_THAT (make_samples (8U),  // Nyquist / 4
               testing::ElementsAre (oscillator::amplitude::fromfp (-1.0),
                                     oscillator::amplitude::fromfp (-0.75),
                                     oscillator::amplitude::fromfp (-0.5),
                                     oscillator::amplitude::fromfp (-0.25),
                                     oscillator::amplitude::fromfp (0.0),
                                     oscillator::amplitude::fromfp (0.25),
                                     oscillator::amplitude::fromfp (0.5),
                                     oscillator::amplitude::fromfp (0.75),
                                     oscillator::amplitude::fromfp (-1.0)));
}
