// -*- mode: c++; coding: utf-8-unix; -*-
#ifndef SYNTH_FIXED_HPP
#define SYNTH_FIXED_HPP

#include <cassert>
#include <ostream>

#include "uint.hpp"

namespace synth {

// Equivalent to (T{1}<<N)-T{1}, where T is an unsigned integer type, but
// without the risk of overflow if N is equal to the number of bits in T.
// Returns 0 if n is 0.
template <unsigned N>
struct mask {
  static constexpr uinteger_t<N> value =
      mask<N - 1U>::value << 1U | uinteger_t<N>{1};
};
template <>
struct mask<0U> {
  static constexpr uinteger_t<1U> value = 0U;
};
template <unsigned N>
inline constexpr auto mask_v = mask<N>::value;

template <unsigned TotalBits, unsigned IntegralBits,
          typename = typename std::enable_if_t<TotalBits >= IntegralBits>>
class fixed {
public:
  static inline constexpr auto total_bits = TotalBits;
  static inline constexpr auto integral_bits = IntegralBits;
  static inline constexpr auto fractional_bits = TotalBits - IntegralBits - 1U;

  using value_type = sinteger_t<total_bits>;
  constexpr fixed () = default;

  static constexpr auto fromfp (double const x) {
    assert (std::abs (x) < (uinteger_t<integral_bits + 1>{1} << integral_bits));
    return fixed{static_cast<value_type> (x * mul_)};
  }
  static constexpr auto fromint (uinteger_t<integral_bits> const integral) {
    return fromint (integral, 0U);
  }
  static constexpr auto fromint (uinteger_t<integral_bits> const integral,
                                 uinteger_t<fractional_bits> const fractional) {
    assert ((integral & ~mask_v<integral_bits>) == 0U);
    assert ((fractional & ~mask_v<fractional_bits>) == 0U);
    return fixed{
        static_cast<value_type> (
            (integral & mask_v<integral_bits>) << fractional_bits) |
        static_cast<value_type> (fractional & mask_v<fractional_bits>)};
  }

  constexpr bool operator== (fixed other) const { return x_ == other.x_; }
  constexpr bool operator!= (fixed other) const { return !operator== (other); }

  constexpr double as_double () const { return x_ / mul_; }
  constexpr decltype (auto) integral_part () const {
    return static_cast<sinteger_t<integral_bits>> (x_ >> fractional_bits);
  }
  constexpr uinteger_t<fractional_bits> fractional_part () const {
    return x_ & mask_v<fractional_bits>;
  }
  constexpr fixed operator+ (fixed const other) const {
    return fixed{x_ + other.x_};
  }
  constexpr fixed operator>> (unsigned const shift) const {
    return fixed{x_ >> shift};
  }
  constexpr fixed operator<< (unsigned const shift) const {
    return fixed{x_ << shift};
  }

  constexpr value_type get () const { return x_; }

private:
  explicit constexpr fixed (value_type const x) : x_{x} {}

  value_type x_ = 0;
  static inline constexpr auto mul_ = static_cast<double> (
      uinteger_t<fractional_bits + 1>{1} << fractional_bits);
};

template <unsigned IntegralBits, unsigned FractionalBits>
inline std::ostream& operator<< (std::ostream& os,
                                 fixed<IntegralBits, FractionalBits> const fp) {
  return os << fp.as_double ();
}

template <unsigned TotalBits, unsigned IntegralBits,
          typename = typename std::enable_if_t<TotalBits >= IntegralBits>>
class ufixed {
public:
  static inline constexpr auto total_bits = TotalBits;
  static inline constexpr auto integral_bits = IntegralBits;
  static inline constexpr auto fractional_bits = total_bits - integral_bits;

  using value_type = uinteger_t<total_bits>;

  constexpr ufixed () = default;
  explicit constexpr ufixed (value_type const x) : x_{x} {}
  static constexpr auto fromfp (double const x) {
    assert (x >= 0.0);
    return ufixed{static_cast<value_type> (std::max (0.0, x) * mul_)};
  }
  static constexpr auto fromint (uinteger_t<integral_bits> const integral) {
    return fromint (integral, 0U);
  }
  static constexpr auto fromint (uinteger_t<integral_bits> const integral,
                                 uinteger_t<fractional_bits> const fractional) {
    assert ((integral & ~mask_v<integral_bits>) == 0U);
    assert ((fractional & ~mask_v<fractional_bits>) == 0U);
    return ufixed{static_cast<value_type> (
        ((integral & mask_v<integral_bits>) << fractional_bits) |
        (fractional & mask_v<fractional_bits>))};
  }

  constexpr bool operator== (ufixed other) const { return x_ == other.x_; }
  constexpr bool operator!= (ufixed other) const { return !operator== (other); }

  constexpr double as_double () const { return x_ / mul_; }
  constexpr uinteger_t<integral_bits> integral_part () const {
    return static_cast<uinteger_t<integral_bits>> (x_ >> fractional_bits);
  }
  constexpr uinteger_t<fractional_bits> fractional_part () const {
    return x_ & mask_v<fractional_bits>;
  }

  constexpr ufixed operator+ (ufixed const other) const {
    return ufixed{x_ + other.x_};
  }
  constexpr ufixed operator>> (unsigned const shift) const {
    return ufixed{x_ >> shift};
  }
  constexpr ufixed operator<< (unsigned const shift) const {
    return ufixed{x_ << shift};
  }

  constexpr value_type get () const { return x_; }

private:
  value_type x_ = 0;
  static inline constexpr auto mul_ = static_cast<double> (
      uinteger_t<fractional_bits + 1>{1} << fractional_bits);
};

template <unsigned IntegralBits, unsigned FractionalBits>
inline std::ostream& operator<< (
    std::ostream& os, ufixed<IntegralBits, FractionalBits> const fp) {
  return os << fp.as_double ();
}

}  // end namespace synth

#endif  // SYNTH_FIXED_HPP
