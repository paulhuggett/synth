#include <gmock/gmock.h>

#include <type_traits>
#include <vector>

#include "synth/nco.hpp"

using namespace synth;

namespace {

struct test_traits {
  /// The number of entries in a wavetable is 2^wavetable_N.
  static constexpr auto wavetable_N = 4U;
  /// Phase accumulation is performed in an M-bit integer register.
  static constexpr auto M = 32U;
};

template <typename Traits>
class wavetable_base {
public:
  using traits = Traits;

  virtual ~wavetable_base () noexcept = default;
  virtual amplitude phase_to_amplitude (
      typename oscillator_info<Traits>::phase_index_type const phase) const = 0;
};

template <typename Traits>
class mock_wavetable : public wavetable_base<Traits> {
public:
  MOCK_METHOD (amplitude, phase_to_amplitude,
               (typename oscillator_info<Traits>::phase_index_type const phase),
               (const, override));
};

// Some values that will appear to have been drawn from our mock wavetables.
// The exact numbers don't matter but should nonetheless be different to one
// another. Here I've used the reciprocal of the first 8 primes just because...
std::vector<amplitude> const results{{
    amplitude::fromfp (1.0 / 2.0),
    amplitude::fromfp (1.0 / 3.0),
    amplitude::fromfp (1.0 / 5.0),
    amplitude::fromfp (1.0 / 7.0),
    amplitude::fromfp (1.0 / 11.0),
    amplitude::fromfp (1.0 / 13.0),
    amplitude::fromfp (1.0 / 17.0),
    amplitude::fromfp (1.0 / 19.0),
}};

}  // end anonymous namespace

template <typename T>
class Oscillator : public testing::Test {
protected:
  void SetUp () override {
    constexpr unsigned sample_rate = T ();
    ASSERT_TRUE (is_power_of_2 (sample_rate))
        << "For this test, sample rate must be a power of 2";
  }

private:
  template <typename UnsignedType, typename = typename std::enable_if_t<
                                       std::is_unsigned_v<UnsignedType>>>
  static constexpr bool is_power_of_2 (UnsignedType const v) {
    return (v & (v - 1U)) == 0U;
  }
};

template <unsigned V>
using unsigned_constant = std::integral_constant<unsigned, V>;

// Exercise the code with a variety of sample rates. These should make no
// difference at all since the requested frequencies are always a whole fraction
// of the sample rate, but I want to exercise the code in this changing
// environment anyway.
using test_sample_rates =
    testing::Types<unsigned_constant<1U << 14>, unsigned_constant<1U << 15>,
                   unsigned_constant<1U << 16>, unsigned_constant<1U << 17>,
                   unsigned_constant<1U << 18>>;
TYPED_TEST_SUITE (Oscillator, test_sample_rates);

TYPED_TEST (Oscillator, DC) {
  using testing::Return;
  using testing::Throw;
  using testing::_;

  using traits = test_traits;
  using phase_index_type = typename oscillator_info<traits>::phase_index_type;
  constexpr unsigned sample_rate = TypeParam ();

  // Define the mock wavetable and set our expectations of it.
  testing::NiceMock<mock_wavetable<traits>> wt;
  ON_CALL (wt, phase_to_amplitude (_))
      .WillByDefault (Throw (std::invalid_argument{"unknown index"}));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (0)))
      .WillByDefault (Return (results.at (0)));

  oscillator<sample_rate, traits, decltype (wt)> osc{&wt};
  osc.set_frequency (frequency::fromint (0));

  EXPECT_EQ (osc.tick (), results.at (0));
  EXPECT_EQ (osc.tick (), results.at (0));
}

TYPED_TEST (Oscillator, Nyquist) {
  using testing::Return;
  using testing::Throw;
  using testing::_;

  using traits = test_traits;
  using phase_index_type = typename oscillator_info<traits>::phase_index_type;

  constexpr unsigned sample_rate = TypeParam ();
  constexpr auto divisor = 2U;
  constexpr auto index = (1U << traits::wavetable_N) / divisor;

  // Define the mock wavetable and set our expectations of it.
  testing::NiceMock<mock_wavetable<traits>> wt;
  ON_CALL (wt, phase_to_amplitude (_))
      .WillByDefault (Throw (std::invalid_argument{"unknown index"}));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (0 * index)))
      .WillByDefault (Return (results.at (0)));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (1 * index)))
      .WillByDefault (Return (results.at (1)));

  oscillator<sample_rate, traits, decltype (wt)> osc{&wt};
  osc.set_frequency (frequency::fromint (sample_rate / divisor));

  // With the frequency set to sample-rate/2, three ticks of the oscillator
  // should yield wavetable values 0, 8, 0.
  EXPECT_EQ (osc.tick (), results.at (0));
  EXPECT_EQ (osc.tick (), results.at (1));
  EXPECT_EQ (osc.tick (), results.at (0));
}

TYPED_TEST (Oscillator, HalfNyquist) {
  using testing::Return;
  using testing::Throw;
  using testing::_;

  using traits = test_traits;
  using phase_index_type = typename oscillator_info<traits>::phase_index_type;

  constexpr unsigned sample_rate = TypeParam ();
  constexpr auto divisor = 4U;
  constexpr auto index = (1U << traits::wavetable_N) / divisor;

  testing::NiceMock<mock_wavetable<traits>> wt;
  ON_CALL (wt, phase_to_amplitude (_))
      .WillByDefault (Throw (std::invalid_argument{"unknown index"}));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (0U * index)))
      .WillByDefault (Return (results.at (0)));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (1U * index)))
      .WillByDefault (Return (results.at (1)));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (2U * index)))
      .WillByDefault (Return (results.at (2)));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (3U * index)))
      .WillByDefault (Return (results.at (3)));

  oscillator<sample_rate, traits, decltype (wt)> osc{&wt};
  osc.set_frequency (frequency::fromint (sample_rate / divisor));

  EXPECT_EQ (osc.tick (), results.at (0));
  EXPECT_EQ (osc.tick (), results.at (1));
  EXPECT_EQ (osc.tick (), results.at (2));
  EXPECT_EQ (osc.tick (), results.at (3));
  EXPECT_EQ (osc.tick (), results.at (0));
}

TYPED_TEST (Oscillator, QuarterNyquist) {
  using testing::_;
  using testing::Return;
  using testing::Throw;

  using traits = test_traits;
  using phase_index_type = typename oscillator_info<traits>::phase_index_type;

  constexpr unsigned sample_rate = TypeParam ();
  constexpr auto divisor = 8U;
  constexpr auto index = (1U << traits::wavetable_N) / divisor;

  // Define the mock wavetable and set our expectations of it.
  testing::NiceMock<mock_wavetable<traits>> wt;
  ON_CALL (wt, phase_to_amplitude (_))
      .WillByDefault (Throw (std::invalid_argument{"unknown index"}));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (0U * index)))
      .WillByDefault (Return (results.at (0)));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (1U * index)))
      .WillByDefault (Return (results.at (1)));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (2U * index)))
      .WillByDefault (Return (results.at (2)));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (3U * index)))
      .WillByDefault (Return (results.at (3)));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (4U * index)))
      .WillByDefault (Return (results.at (4)));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (5U * index)))
      .WillByDefault (Return (results.at (5)));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (6U * index)))
      .WillByDefault (Return (results.at (6)));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (7U * index)))
      .WillByDefault (Return (results.at (7)));

  oscillator<sample_rate, traits, decltype (wt)> osc{&wt};
  osc.set_frequency (frequency::fromint (sample_rate / divisor));

  EXPECT_EQ (osc.tick (), results.at (0));
  EXPECT_EQ (osc.tick (), results.at (1));
  EXPECT_EQ (osc.tick (), results.at (2));
  EXPECT_EQ (osc.tick (), results.at (3));
  EXPECT_EQ (osc.tick (), results.at (4));
  EXPECT_EQ (osc.tick (), results.at (5));
  EXPECT_EQ (osc.tick (), results.at (6));
  EXPECT_EQ (osc.tick (), results.at (7));
  EXPECT_EQ (osc.tick (), results.at (0));
}

TEST (Oscillator, NyquistByThree) {
  using testing::_;
  using testing::Return;
  using testing::Throw;

  using traits = test_traits;
  using phase_index_type = typename oscillator_info<traits>::phase_index_type;

  // Define the mock wavetable and set our expectations of it.
  testing::NiceMock<mock_wavetable<traits>> wt;
  ON_CALL (wt, phase_to_amplitude (_))
      .WillByDefault (Throw (std::invalid_argument{"unknown index"}));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (0U)))
      .WillByDefault (Return (results.at (0)));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (0x5, 0x553C000)))
      .WillByDefault (Return (results.at (1)));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (0x5, 0x54F0000)))
      .WillByDefault (Return (results.at (2)));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (0xA, 0xAA78000)))
      .WillByDefault (Return (results.at (3)));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (0xA, 0xAA2C000)))
      .WillByDefault (Return (results.at (4)));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (0xF, 0xFFB4000)))
      .WillByDefault (Return (results.at (5)));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (0xF, 0xFFB4000)))
      .WillByDefault (Return (results.at (6)));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (0xf, 0xff68000)))
      .WillByDefault (Return (results.at (7)));

  constexpr unsigned sample_rate = 48000U;
  oscillator<sample_rate, traits, decltype (wt)> osc{&wt};
  osc.set_frequency (frequency::fromint (sample_rate / 3U));

  // Run the oscillator through a few ticks and check the output.
  EXPECT_EQ (osc.tick (), results.at (0));
  EXPECT_EQ (osc.tick (), results.at (1));
  EXPECT_EQ (osc.tick (), results.at (3));
  EXPECT_EQ (osc.tick (), results.at (6));
  EXPECT_EQ (osc.tick (), results.at (2));
  EXPECT_EQ (osc.tick (), results.at (4));
  EXPECT_EQ (osc.tick (), results.at (7));
}
