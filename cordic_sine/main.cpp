// -*- mode: c++; coding: utf-8-unix; -*-
//
// Cordic in 32-bit signed fixed point math
//
// Derived from "Matters Computational Ideas, Algorithms, Source Code" by Jörg
// Arndt Section 33.2 "CORDIC algorithms"
#include <array>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <iostream>

// Constants
#ifdef M_PI
constexpr auto pi = M_PI;
#else
constexpr auto pi = 3.14159265358979323846264338327950288;
#endif
#ifdef M_PI_2
constexpr auto half_pi = M_PI_2;  // pi/2
#else
constexpr auto half_pi = pi / 2.0;
#endif

class convert {
public:
  static constexpr auto bits = 32U;

  static constexpr double tofp (int32_t const x) { return x / mul; }
  static constexpr int32_t fromfp (double const x) {
    return static_cast<int32_t> (x * mul);
  }

private:
  static constexpr auto mul = static_cast<double> (1U << (bits - 2U));
};

constexpr auto iterations = 32U;
static_assert (iterations <= convert::bits);

namespace {

void check_ctab (std::array<int32_t, convert::bits> const& ctab) {
  (void)ctab;
#ifndef NDEBUG
  auto k = 0;
  for (auto const c : ctab) {
    assert (c == convert::fromfp (std::atan (std::pow (2, -k))));
    ++k;
  }
#endif
}

// Compute the scaling constant K
//
// K = 1.646760258121065648366051222282298435652376725701027409…
// 1/K = 0.6072529350088812561694467525049282631123908521500897724…
//
//     ∞
// K = 𝚷  √(1+2^(-2k))
//     k=0
//
// It can be computed more efficiently as K=√(2F(1/4)) where F(x) is defined as
//         ∞
//  F(z) = 𝚷 (1+z^k)
//         k=1
#ifndef NDEBUG
double scaling_constant () {
  // The definition of F() uses a product of terms from 1…∞. Just evaluating the
  // first few is sufficient.
  constexpr auto terms = 16U;
  auto F = 1.0;
  auto z = 0.25;
  for (auto k = 1U; k <= terms; ++k) {
    F *= 1.0 + std::pow (z, k);
  }
  return std::sqrt (2 * F);
}
#endif

// The lookup table has to contain the values arctan(2^-k) for k=[0,bits), these
// are stored in the array cordic_ctab[].
constexpr std::array<int32_t, convert::bits> cordic_ctab{{
    0x3243F6A8, 0x1DAC6705, 0x0FADBAFC, 0x07F56EA6, 0x03FEAB76, 0x01FFD55B,
    0x00FFFAAA, 0x007FFF55, 0x003FFFEA, 0x001FFFFD, 0x000FFFFF, 0x0007FFFF,
    0x0003FFFF, 0x0001FFFF, 0x0000FFFF, 0x00007FFF, 0x00003FFF, 0x00001FFF,
    0x00000FFF, 0x000007FF, 0x000003FF, 0x000001FF, 0x000000FF, 0x0000007F,
    0x0000003F, 0x0000001F, 0x0000000F, 0x00000008, 0x00000004, 0x00000002,
    0x00000001, 0x00000000,
}};

// Function is valid for arguments in range [-π/2,π/2]
std::pair<int32_t, int32_t> cordic (int32_t const theta) {
  static constexpr int32_t reciprocal_K =
      INT32_C (0x26DD3B6A);  // This is 1/K in 32-bit fixed point.
  assert (reciprocal_K == convert::fromfp (1.0 / scaling_constant ()));

  auto x = reciprocal_K;
  auto y = int32_t{0};
  auto z = theta;
  for (auto k = 0U; k < iterations; ++k) {
    // Extract the sign bit from z. Could be written as:
    // int32_t const d = z >= 0 ? 0 : -1;
    int32_t const d = z >> 31;

    int32_t const tx = x - (((y >> k) ^ d) - d);
    int32_t const ty = y + (((x >> k) ^ d) - d);
    int32_t const tz = z - ((cordic_ctab[k] ^ d) - d);
    x = tx;
    y = ty;
    z = tz;
  }
  return std::make_pair (y /*sine*/, x /*cosine*/);
}

double check () {
  // Test the range [-π/2,π/2].
  static constexpr auto steps = 50;
  auto max_difference = 0.0;
  for (auto i = -steps; i <= steps; ++i) {
    auto const theta = (static_cast<double> (i) / steps) * half_pi;
    auto const [s, _] = cordic (convert::fromfp (theta));

    auto const cordic_sin = convert::tofp (s);
    auto const stdlib_sin = std::sin (theta);
    // These values should be nearly equal
    auto const difference = std::abs (cordic_sin - stdlib_sin);
    max_difference = std::max (max_difference, difference);
  }
  return max_difference;
}

}  // end anonymous namespace

// Print out sin(x) vs fp CORDIC sin(x)
int main () {
  check_ctab (cordic_ctab);
  check ();

  // Adjust the angle so that it lies within the range supported by the CORDIC
  // algorithm
  // ([-π/2,π/2]). For values (π/2,π], f(θ) = π/2-(θ-π/2) and similarly for
  // values [-π,-π/2).
  auto const adjust = [] (double theta) {
    theta = std::fmod (theta, pi);
    if (theta < -half_pi) {
      return -half_pi - (theta + half_pi);
    }
    if (theta > half_pi) {
      return half_pi - (theta - half_pi);
    }
    return theta;
  };

  constexpr auto steps = 200U;
  for (auto ctr = 0U; ctr <= steps; ++ctr) {
    auto const theta = static_cast<double> (ctr * 2U) * pi / steps - pi;
    auto const [s, c] = cordic (convert::fromfp (adjust (theta)));
    std::cout << theta << ' ' << convert::tofp (s) << '\n';
  }
}
