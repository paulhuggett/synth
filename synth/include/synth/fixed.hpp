// -*- mode: c++; coding: utf-8-unix; -*-
#ifndef SYNTH_FIXED_HPP
#define SYNTH_FIXED_HPP

#include <cassert>
#include <cmath>
#include <ostream>

#include "synth/uint.hpp"

namespace synth {

// Equivalent to (T{1}<<N)-T{1}, where T is an unsigned integer type, but
// without the risk of overflow if N is equal to the number of bits in T.
// Returns 0 if n is 0.
template <unsigned Bits>
struct mask {
  static constexpr uinteger_t<Bits> value =
      uinteger_t<Bits>{mask<Bits - 1U>::value} << 1U | uinteger_t<Bits>{1};
};
template <>
struct mask<0U> {
  static constexpr uinteger_t<1U> value = 0U;
};
template <unsigned Bits>
inline constexpr auto mask_v = mask<Bits>::value;

template <unsigned TotalBits, unsigned IntegralBits,
          typename = typename std::enable_if_t<TotalBits >= IntegralBits>>
class fixed {
public:
  static constexpr auto total_bits = TotalBits;
  static constexpr auto integral_bits = IntegralBits;
  static constexpr auto fractional_bits = TotalBits - IntegralBits - 1U;

  using value_type = sinteger_t<total_bits>;
  constexpr fixed () = default;

  static constexpr decltype (auto) fromfp (double const x) {
    assert (std::isfinite (x) &&
            std::abs (x) < (uinteger_t<integral_bits + 1>{1} << integral_bits));
    return fixed{static_cast<value_type> (std::round (x * mul_))};
  }
  static constexpr fixed frombits (uinteger_t<total_bits> const bits) {
    return fixed{static_cast<value_type> (bits)};
  }
  static constexpr decltype (auto) fromint (
      uinteger_t<integral_bits> const integral) {
    return fromint (integral, 0U);
  }
  static constexpr decltype (auto) fromint (
      uinteger_t<integral_bits> const integral,
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
  static constexpr auto mul_ = static_cast<double> (
      uinteger_t<fractional_bits + 1>{1} << fractional_bits);
};

template <unsigned IntegralBits, unsigned FractionalBits>
inline std::ostream& operator<< (std::ostream& os,
                                 fixed<IntegralBits, FractionalBits> const fp) {
  return os << fp.as_double ();
}

template <size_t WL, size_t IWL,
          typename = typename std::enable_if_t<WL >= IWL>>
class ufixed;

/// \tparam WL  The word length, which shall be the total number of bits in the number
///   representation.
/// \tparam IWL  The integer word length, which shall be the number of bits in the integer part (the
///   position of the binary point relative to the left-most bit).
template <size_t WL, size_t IWL>
class ufixed<WL, IWL> {
  template <size_t OtherWL, size_t OtherIWL, typename>
  friend class ufixed;

public:
  static constexpr auto total_bits = WL;
  static constexpr auto integral_bits = IWL;
  static constexpr auto fractional_bits = WL - IWL;

  using value_type = uinteger_t<total_bits>;
  constexpr ufixed () = default;

  template <size_t OtherWL, size_t OtherIWL>
  constexpr ufixed<OtherWL, OtherIWL> cast () const;

  template <typename OtherFixed>
  constexpr OtherFixed cast () const;

  static constexpr decltype (auto) fromfp (double const x) {
#if __has_builtin(__builtin_isfinite)
    assert (__builtin_isfinite (x));
#endif
    assert (x >= 0.0);
    return ufixed{static_cast<value_type> (std::max (0.0, x) * mul_)};
  }
  static constexpr decltype (auto) frombits (
      uinteger_t<total_bits> const bits) {
    return ufixed{static_cast<value_type> (bits)};
  }
  static constexpr ufixed fromint (uinteger_t<integral_bits> const integral) {
    return fromint (integral, uinteger_t<fractional_bits>{0});
  }
  static constexpr ufixed fromint (uinteger_t<integral_bits> integral,
                                   uinteger_t<fractional_bits> fractional);

  /// Member function is_neg shall return true if the object holds a negative
  /// value; otherwise, the return value shall be false.
  constexpr bool is_neg () const {
    return false;
  }
  /// Member function is_zero shall return true if the object holds a zero
  /// value; otherwise, the return value shall be false.
  constexpr bool is_zero () const {
    return x_ == 0;
  }

  constexpr bool operator== (ufixed other) const { return x_ == other.x_; }
  constexpr bool operator!= (ufixed other) const { return !operator== (other); }
  constexpr bool operator> (ufixed const other) const {
    return x_ > other.x_;
  }
  constexpr bool operator>= (ufixed const other) const {
    return x_ >= other.x_;
  }

  constexpr explicit operator double () const {
    return x_ / mul_;
  }
  constexpr decltype (auto) integral_part () const {
    return static_cast<uinteger_t<integral_bits>> (x_ >> fractional_bits);
  }
  constexpr uinteger_t<fractional_bits> fractional_part () const {
    return x_ & mask_v<fractional_bits>;
  }

  constexpr ufixed operator>> (unsigned const shift) const {
    return ufixed{x_ >> shift};
  }
  constexpr ufixed operator<< (unsigned const shift) const {
    return ufixed{x_ << shift};
  }

  constexpr value_type get () const { return x_; }

private:
  explicit constexpr ufixed (value_type const x) : x_{x} {
  }

  value_type x_ = 0;
  static constexpr auto mul_ = static_cast<double> (
      uinteger_t<fractional_bits + 1>{1} << fractional_bits);
};

// fromint
// ~~~~~~~
template <size_t WL, size_t IWL>
constexpr ufixed<WL, IWL> ufixed<WL, IWL>::fromint (
    uinteger_t<integral_bits> const integral,
    uinteger_t<fractional_bits> const fractional) {
  assert ((integral & ~mask_v<integral_bits>) == 0U);
  assert ((fractional & ~mask_v<fractional_bits>) == 0U);
  return ufixed{static_cast<value_type> (
                    (integral & mask_v<integral_bits>) << fractional_bits) |
                static_cast<value_type> (fractional & mask_v<fractional_bits>)};
}

// cast
// ~~~~
template <size_t WL, size_t IWL>
template <size_t OtherWL, size_t OtherIWL>
constexpr ufixed<OtherWL, OtherIWL> ufixed<WL, IWL>::cast () const {
  using result_type = ufixed<OtherWL, OtherIWL>;
  constexpr auto mask = mask_v<result_type::total_bits>;
  if constexpr (result_type::fractional_bits > fractional_bits) {
    return result_type{static_cast<typename result_type::value_type> (
        (x_ << (result_type::fractional_bits - fractional_bits)) & mask)};
  } else if constexpr (result_type::fractional_bits <= fractional_bits) {
    return result_type{static_cast<typename result_type::value_type> (
        (x_ >> (fractional_bits - result_type::fractional_bits)) & mask)};
  }
}

template <size_t WL, size_t IWL>
template <typename OtherFixed>
constexpr OtherFixed ufixed<WL, IWL>::cast () const {
  return this->cast<OtherFixed::total_bits, OtherFixed::integral_bits> ();
}

// U(a, b) + U(a, b) = U(a + 1, b)
template <size_t WL, size_t IWL>
constexpr decltype (auto) operator+ (ufixed<WL, IWL> const lhs,
                                     ufixed<WL, IWL> const rhs) {
  using result_type = ufixed<WL + 1, IWL + 1>;
  return result_type::frombits (
      static_cast<typename result_type::value_type> (lhs.get ()) + rhs.get ());
}

template <size_t WL, size_t IWL>
constexpr decltype (auto) operator- (ufixed<WL, IWL> const lhs,
                                     ufixed<WL, IWL> const rhs) {
  using result_type = ufixed<WL, IWL>;
  return result_type::frombits (lhs.get () - rhs.get ());
}

// U(a_1, b_1) * U(a_2, b_2) = U(a_1 + a_2, b_1 + b_2)
template <size_t LhsWL, size_t LhsIWL, size_t RhsWL, size_t RhsIWL>
constexpr decltype (auto) operator* (ufixed<LhsWL, LhsIWL> const lhs,
                                     ufixed<RhsWL, RhsIWL> const rhs) {
  using result_type = ufixed<LhsWL + RhsWL, LhsIWL + RhsIWL>;
  return result_type::frombits (
      static_cast<typename result_type::value_type> (lhs.get ()) * rhs.get ());
}

}  // end namespace synth

#endif  // SYNTH_FIXED_HPP
