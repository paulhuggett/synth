// -*- mode: c++; coding: utf-8-unix; -*-
#ifndef SYNTH_UINT_HPP
#define SYNTH_UINT_HPP

#include <cstdint>
#include <type_traits>

namespace synth {

/// Yields the smallest unsigned integer type with at least \p Bits bits.
template <size_t Bits>
struct uinteger {
  using type = typename uinteger<Bits + 1>::type;
};
template <size_t Bits>
using uinteger_t = typename uinteger<Bits>::type;
template <>
struct uinteger<8> {
  using type = uint8_t;
};
template <>
struct uinteger<16> {
  using type = uint16_t;
};
template <>
struct uinteger<32> {
  using type = uint32_t;
};
template <>
struct uinteger<64> {
  using type = uint64_t;
};

/// Yields the smallest signed integer type with at least \p Bits bits.
template <size_t Bits>
struct sinteger {
  using type = typename sinteger<Bits + 1>::type;
};
template <size_t Bits>
using sinteger_t = typename sinteger<Bits>::type;
template <>
struct sinteger<8> {
  using type = int8_t;
};
template <>
struct sinteger<16> {
  using type = int16_t;
};
template <>
struct sinteger<32> {
  using type = int32_t;
};
template <>
struct sinteger<64> {
  using type = int64_t;
};

static_assert (std::is_same_v<uinteger_t<1>, uint8_t>);
static_assert (std::is_same_v<uinteger_t<7>, uint8_t>);
static_assert (std::is_same_v<uinteger_t<8>, uint8_t>);
static_assert (std::is_same_v<uinteger_t<9>, uint16_t>);

static_assert (std::is_same_v<uinteger_t<15>, uint16_t>);
static_assert (std::is_same_v<uinteger_t<16>, uint16_t>);
static_assert (std::is_same_v<uinteger_t<17>, uint32_t>);

static_assert (std::is_same_v<uinteger_t<31>, uint32_t>);
static_assert (std::is_same_v<uinteger_t<32>, uint32_t>);

static_assert (std::is_same_v<uinteger_t<33>, uint64_t>);
static_assert (std::is_same_v<uinteger_t<63>, uint64_t>);
static_assert (std::is_same_v<uinteger_t<64>, uint64_t>);

static_assert (std::is_same_v<sinteger_t<1>, int8_t>);
static_assert (std::is_same_v<sinteger_t<7>, int8_t>);
static_assert (std::is_same_v<sinteger_t<8>, int8_t>);
static_assert (std::is_same_v<sinteger_t<9>, int16_t>);

static_assert (std::is_same_v<sinteger_t<15>, int16_t>);
static_assert (std::is_same_v<sinteger_t<16>, int16_t>);
static_assert (std::is_same_v<sinteger_t<17>, int32_t>);

static_assert (std::is_same_v<sinteger_t<31>, int32_t>);
static_assert (std::is_same_v<sinteger_t<32>, int32_t>);

static_assert (std::is_same_v<sinteger_t<33>, int64_t>);
static_assert (std::is_same_v<sinteger_t<63>, int64_t>);
static_assert (std::is_same_v<sinteger_t<64>, int64_t>);

}  // end namespace synth

#endif  // SYNTH_UINT_HPP
