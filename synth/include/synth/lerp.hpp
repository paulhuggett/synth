#ifndef SYNTH_LERP_HPP
#define SYNTH_LERP_HPP

#include "synth/fixed.hpp"

namespace synth {

namespace details {

static constexpr bool EARLY_RETURN = false;

template <size_t N, typename T>
constexpr T lerp_a (T hi, T lo, T ratio) {
  T const mid = (hi + lo) / T{2};
  if constexpr (N == 0U) {
    return mid;
  } else {
    if (ratio == T{1}) {
      if constexpr (EARLY_RETURN) {
        return hi;
      } else {
        lo = hi;
      }
    } else if (ratio == T{0}) {
      if constexpr (EARLY_RETURN) {
        return lo;
      } else {
        hi = lo;
      }
    } else if (ratio >= 0.5) {
      lo = mid;
      ratio -= 0.5;
    } else {
      hi = mid;
    }
    return lerp_a<N - 1> (hi, lo, ratio * T{2});
  }
}

template <size_t N, size_t WL, size_t IWL>
constexpr ufixed<WL, IWL> lerp_a (ufixed<WL, IWL> hi, ufixed<WL, IWL> lo,
                                  ufixed<WL, IWL> ratio) {
  auto const mid = ((hi + lo) >> 1U).template cast<WL, IWL> ();
  if constexpr (N == 0U) {
    return mid;
  } else {
    constexpr auto zero = ufixed<WL, IWL>::fromint (0U);
    constexpr auto one = ufixed<WL, IWL>::fromint (1U);
    constexpr auto half = one >> 1U;

    if (ratio == one) {
      if constexpr (EARLY_RETURN) {
        return hi;
      } else {
        lo = hi;
      }
    } else if (ratio == zero) {
      if constexpr (EARLY_RETURN) {
        return lo;
      } else {
        hi = lo;
      }
    } else if (ratio >= half) {
      lo = mid;
      ratio = ratio - half;
    } else {
      hi = mid;
    }
    return lerp_a<N - 1> (hi, lo, ratio << 1U);
  }
}

}  // end namespace details

template <typename T>
constexpr T lerp_a (T hi, T lo, T ratio) {
  return details::lerp_a<5> (hi, lo, ratio);
}

template <typename T>
constexpr T lerp_b (T hi, T lo, T ratio) {
  return (hi - lo) * ratio + lo;
}

/// \tparam WL  The word length, which shall be the total number of bits in the number
///   representation.
/// \tparam IWL  The integer word length, which shall be the number of bits in the integer part (the
///   position of the binary point relative to the left-most bit).
template <size_t WL, size_t IWL>
constexpr ufixed<WL, IWL> lerp_b (ufixed<WL, IWL> const hi,
                                  ufixed<WL, IWL> const lo,
                                  ufixed<WL, IWL> const ratio) {
  return (((hi - lo) * ratio).template cast<WL, IWL> () + lo)
      .template cast<WL, IWL> ();
}

#if 0
int main (/*int argc, char const * argv[]*/) {
  using ftype = ufixed<24, 3>;

  constexpr auto sixteenth = ftype::fromint (1) >> 4;
  assert (sixteenth == ftype::fromfp (1.0 / 16));
  constexpr auto thirtysecond = ftype::fromint (1) >> 5;
  assert (thirtysecond == ftype::fromfp (1.0 / 32));

  auto ratio = ftype::fromint (0);
  std::cout << std::left << std::setprecision (2);
  for (auto ctr = 0U; ctr <= 32; ++ctr) {
    static constexpr auto hi = ftype::fromint (4);
    static constexpr auto lo = ftype::fromint (1);

    auto const v1 = lerp_a (hi, lo, ratio);
    auto const v2 = lerp_b (hi, lo, ratio);

    static constexpr auto width = 5;
    std::cout << hi << ' ' << lo << ' '
              << std::setw (width) << ratio
              << " v1:" << std::setw (width) << v1
              << " v2:" << std::setw (width) << v2 << '\n';
    ratio = (ratio + thirtysecond).cast<ftype::total_bits, ftype::integral_bits>();
  }
  return 0;
}
#endif

}  // end namespace synth

#endif  // SYNTH_LERP_HPP
