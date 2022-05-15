#include <gmock/gmock.h>

#include <type_traits>
#include <vector>

#include "synth/nco.hpp"

using namespace synth;

#if 0
TEST_F (Oscillator, SampleRateOverSix) {
  auto const wavetable_entry = [this] (unsigned const index) {
    assert (index < (size_t{1} << decltype (wt_)::traits::wavetable_N));
    auto it = wt_.begin ();
    std::advance (it, index);
    return *it;
  };
  EXPECT_THAT (
      make_samples (6U),
      testing::ElementsAre (wavetable_entry (0U), wavetable_entry (341U),
                            wavetable_entry (682U), wavetable_entry (1023U),
                            wavetable_entry (1365U), wavetable_entry (1706U),
                            wavetable_entry (2047U)));
}
#endif

namespace {

struct test_traits {
  /// The number of entries in a wavetable is 2^wavetable_N.
  static inline constexpr auto wavetable_N = 4U;
  /// Phase accumulation is performed in an M-bit integer register.
  static inline constexpr auto M = 32U;
};

template <typename Traits>
class wavetable_base {
public:
  virtual amplitude phase_to_amplitude (
      typename oscillator_info<Traits>::phase_index_type const phase)
      const noexcept = 0;
};

template <typename Traits>
class mock_wavetable : public wavetable_base<Traits> {
public:
  MOCK_METHOD (amplitude, phase_to_amplitude,
               (typename oscillator_info<Traits>::phase_index_type const phase),
               (const, noexcept, override));
};

}  // namespace

template <typename T>
class Oscillator : public testing::Test {
protected:
  void SetUp () override {
    constexpr unsigned sample_rate = T ();
    ASSERT_TRUE (is_power_of_2 (sample_rate))
        << "For this test, sample rate must be a power of 2";
  }

  static std::vector<amplitude> const results;

private:
  template <typename UnsignedType, typename = typename std::enable_if_t<
                                       std::is_unsigned_v<UnsignedType>>>
  constexpr bool is_power_of_2 (UnsignedType const v) {
    return (v & (v - 1U)) == 0U;
  }
};

// Some values that will appear to have been drawn from our mock wavetables.
// The exact numbers don't matter but should nonetheless be different to one
// another. Here I've used the reciprocal of the first 8 primes just because...
template <typename T>
std::vector<amplitude> const Oscillator<T>::results{{
    amplitude::fromint (0, 0x20000000),  // 1/2=0.5
    amplitude::fromint (0, 0x15555555),  // 1/3=0.333333
    amplitude::fromint (0, 0xccccccc),   // 1/5=0.2
    amplitude::fromint (0, 0x9249249),   // 1/7=0.142857
    amplitude::fromint (0, 0x5d1745d),   // 1/11=0.0909091
    amplitude::fromint (0, 0x4ec4ec4),   // 1/13=0.0769231
    amplitude::fromint (0, 0x3c3c3c3),   // 1/17=0.0588235
    amplitude::fromint (0, 0x35e50d7),   // 1/19=0.0526316
}};

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
  using traits = test_traits;
  using phase_index_type = typename oscillator_info<traits>::phase_index_type;
  constexpr unsigned sample_rate = TypeParam ();

  // Define the mock wavetable and set our expectations of it.
  testing::NiceMock<mock_wavetable<traits>> wt;
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (0)))
      .WillByDefault (Return (TestFixture::results[0]));

  oscillator<sample_rate, traits, decltype (wt)> osc{&wt};
  osc.set_frequency (frequency::fromint (0));

  EXPECT_EQ (osc.tick (), TestFixture::results[0]);
  EXPECT_EQ (osc.tick (), TestFixture::results[0]);
}

TYPED_TEST (Oscillator, Nyquist) {
  using testing::Return;
  using traits = test_traits;
  using phase_index_type = typename oscillator_info<traits>::phase_index_type;

  constexpr unsigned sample_rate = TypeParam ();
  constexpr auto divisor = 2U;
  constexpr auto index = (1U << traits::wavetable_N) / divisor;

  // Define the mock wavetable and set our expectations of it.
  testing::NiceMock<mock_wavetable<traits>> wt;
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (0 * index)))
      .WillByDefault (Return (TestFixture::results[0]));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (1 * index)))
      .WillByDefault (Return (TestFixture::results[1]));

  oscillator<sample_rate, traits, decltype (wt)> osc{&wt};
  osc.set_frequency (frequency::fromint (sample_rate / divisor));

  // With the frequency set to sample-rate/2, three ticks of the oscillator
  // should yield wavetable values 0, 8, 0.
  EXPECT_EQ (osc.tick (), TestFixture::results[0]);
  EXPECT_EQ (osc.tick (), TestFixture::results[1]);
  EXPECT_EQ (osc.tick (), TestFixture::results[0]);
}

TYPED_TEST (Oscillator, HalfNyquist) {
  using testing::Return;
  using traits = test_traits;
  using phase_index_type = typename oscillator_info<traits>::phase_index_type;

  constexpr unsigned sample_rate = TypeParam ();
  constexpr auto divisor = 4U;
  constexpr auto index = (1U << traits::wavetable_N) / divisor;

  testing::NiceMock<mock_wavetable<traits>> wt;
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (0U * index)))
      .WillByDefault (Return (TestFixture::results[0]));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (1U * index)))
      .WillByDefault (Return (TestFixture::results[1]));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (2U * index)))
      .WillByDefault (Return (TestFixture::results[2]));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (3U * index)))
      .WillByDefault (Return (TestFixture::results[3]));

  oscillator<sample_rate, traits, decltype (wt)> osc{&wt};
  osc.set_frequency (frequency::fromint (sample_rate / divisor));

  EXPECT_EQ (osc.tick (), TestFixture::results[0]);
  EXPECT_EQ (osc.tick (), TestFixture::results[1]);
  EXPECT_EQ (osc.tick (), TestFixture::results[2]);
  EXPECT_EQ (osc.tick (), TestFixture::results[3]);
  EXPECT_EQ (osc.tick (), TestFixture::results[0]);
}

TYPED_TEST (Oscillator, QuarterNyquist) {
  using testing::Return;
  using traits = test_traits;
  using phase_index_type = typename oscillator_info<traits>::phase_index_type;

  constexpr unsigned sample_rate = TypeParam ();
  constexpr auto divisor = 8U;
  constexpr auto index = (1U << traits::wavetable_N) / divisor;

  // Define the mock wavetable and set our expectations of it.
  testing::NiceMock<mock_wavetable<traits>> wt;
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (0U * index)))
      .WillByDefault (Return (TestFixture::results[0]));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (1U * index)))
      .WillByDefault (Return (TestFixture::results[1]));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (2U * index)))
      .WillByDefault (Return (TestFixture::results[2]));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (3U * index)))
      .WillByDefault (Return (TestFixture::results[3]));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (4U * index)))
      .WillByDefault (Return (TestFixture::results[4]));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (5U * index)))
      .WillByDefault (Return (TestFixture::results[5]));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (6U * index)))
      .WillByDefault (Return (TestFixture::results[6]));
  ON_CALL (wt, phase_to_amplitude (phase_index_type::fromint (7U * index)))
      .WillByDefault (Return (TestFixture::results[7]));

  oscillator<sample_rate, traits, decltype (wt)> osc{&wt};
  osc.set_frequency (frequency::fromint (sample_rate / divisor));

  EXPECT_EQ (osc.tick (), TestFixture::results[0]);
  EXPECT_EQ (osc.tick (), TestFixture::results[1]);
  EXPECT_EQ (osc.tick (), TestFixture::results[2]);
  EXPECT_EQ (osc.tick (), TestFixture::results[3]);
  EXPECT_EQ (osc.tick (), TestFixture::results[4]);
  EXPECT_EQ (osc.tick (), TestFixture::results[5]);
  EXPECT_EQ (osc.tick (), TestFixture::results[6]);
  EXPECT_EQ (osc.tick (), TestFixture::results[7]);
  EXPECT_EQ (osc.tick (), TestFixture::results[0]);
}
