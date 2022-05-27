// -*- mode: c++; coding: utf-8-unix; -*-
#ifndef SYNTH_WAV_FILE_HPP
#define SYNTH_WAV_FILE_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <type_traits>

namespace synth {

namespace details {

/// Writes a FourCC ("four-character code" AKA OSType) to an output iterator.
///
/// \tparam OutputIterator  A type compatible with the requirements of an output
///   iterator.
/// \param c0  The first of the four characters to be written.
/// \param c1  The second of the four characters to be written.
/// \param c2  The third of the four characters to be written.
/// \param c3  The fourth of the four characters to be written.
/// \param it  The output iterator to which the four bytes are written.
/// \returns  The output iterator \p it after writing the four bytes.
template <typename OutputIterator>
OutputIterator append4cc (char const c0, char const c1, char const c2,
                          char const c3, OutputIterator it) {
  *(it++) = static_cast<uint8_t> (c0);
  *(it++) = static_cast<uint8_t> (c1);
  *(it++) = static_cast<uint8_t> (c2);
  *(it++) = static_cast<uint8_t> (c3);
  return it;
}

/// A function object which will write an unsigned integer of consisting of a
/// specified number of bits in little-endian order to an output iterator.
///
/// \tparam Bits The number of bits of output to produce.
/// \tparam OutputIterator  A type compatible with the requirements of an output
///   iterator.
template <unsigned Bits, typename OutputIterator>
struct appender {
  /// \param v  The value to be written.
  /// \param it  The output iterator to which data will be written.
  /// \returns  The output iterator \p it after writing.
  OutputIterator operator() (uinteger_t<Bits> const v, OutputIterator it) const;
};
template <typename OutputIterator>
struct appender<8, OutputIterator> {
  OutputIterator operator() (uint8_t const v, OutputIterator it) const {
    *(it++) = v;
    return it;
  }
};
template <typename OutputIterator>
struct appender<16, OutputIterator> {
  OutputIterator operator() (uint16_t const v, OutputIterator it) const {
    *(it++) = static_cast<uint8_t> (v);
    *(it++) = static_cast<uint8_t> (v >> 8);
    return it;
  }
};
template <typename OutputIterator>
struct appender<24, OutputIterator> {
  OutputIterator operator() (uint32_t const v, OutputIterator it) const {
    *(it++) = static_cast<uint8_t> (v);
    *(it++) = static_cast<uint8_t> (v >> 8);
    *(it++) = static_cast<uint8_t> (v >> 16);
    return it;
  }
};
template <typename OutputIterator>
struct appender<32, OutputIterator> {
  OutputIterator operator() (uint32_t const v, OutputIterator it) const {
    *(it++) = static_cast<uint8_t> (v);
    *(it++) = static_cast<uint8_t> (v >> 8);
    *(it++) = static_cast<uint8_t> (v >> 16);
    *(it++) = static_cast<uint8_t> (v >> 24);
    return it;
  }
};

/// \tparam Bits The number of bits of output to produce.
/// \tparam OutputIterator  A type compatible with the requirements of an output
///   iterator.
/// \param v  The value to be written.
/// \param it  The beginning of the destination range.
/// \returns  Output iterator to the element in the destination range, one past
///   the last value written.
template <unsigned Bits, typename OutputIterator>
inline OutputIterator append (uinteger_t<Bits> const v,
                              OutputIterator const it) {
  return appender<Bits, OutputIterator>{}(v, it);
}
/// \tparam Bits The number of bits of output to produce.
/// \tparam OutputIterator  A type compatible with the requirements of an output
///   iterator.
/// \param v  The value to be written.
/// \param it  The beginning of the destination range.
/// \returns  Output iterator to the element in the destination range, one past
///   the last value written.
template <unsigned Bits, typename OutputIterator>
inline OutputIterator append (sinteger_t<Bits> const v,
                              OutputIterator const it) {
  return append<Bits> (static_cast<uinteger_t<Bits>> (v), it);
}

/// Converts a floating point value \p x, which will be clamped to the range
/// [-1,+1], to a signed integer in the range [-2^Bits, 2^Bits). The returned
/// type is a signed integer of at least \p Bits bits.
///
/// \tparam Bits  The number of bits in the output value.
/// \param x  The value to be scaled.
/// \returns  The scaled equivalent of \p x.
template <unsigned Bits, typename T = sinteger_t<Bits>>
inline T to_integral (double const x) {
  assert (std::isfinite (x));
  static constexpr auto scale = ((uinteger_t<Bits + 1>{1} << Bits) - 1U) / 2.0;
  return static_cast<T> (
      std::floor (std::max (std::min (x, 1.0), -1.0) * scale));
}

/// \tparam ForwardIterator type compatible with the requirements of a forward
/// iterator.
/// \param first  The iterator pointing to the first element.
/// \param last   Iterator pointing to the end of the range.
/// \param max  The largest allowed return value.
/// \returns  The distance between the \p first and \p last iterators as an
///   unsigned 32 bit value. If the distance is negative, it is clamped to zero.
///   The maximum value is clamped to \p max.
template <typename ForwardIterator>
constexpr uint32_t clamped_distance (
    ForwardIterator const first, ForwardIterator const last,
    uint32_t const max = std::numeric_limits<uint32_t>::max ()) {
  using diff_type =
      typename std::iterator_traits<ForwardIterator>::difference_type;
  using udiff_type = std::make_unsigned_t<diff_type>;
  using widest_type =
      std::conditional_t<(std::numeric_limits<uint32_t>::digits >
                          std::numeric_limits<udiff_type>::digits),
                         uint32_t, udiff_type>;
  return static_cast<uint32_t> (
      std::min (static_cast<widest_type> (
                    std::max (std::distance (first, last), diff_type{0})),
                static_cast<widest_type> (max)));
}

template <typename OutputIterator, unsigned Bits>
class append_iterator {
public:
  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = void;
  using pointer = void;
  using reference = void;

  explicit constexpr append_iterator (OutputIterator const& it) : it_{it} {}
  OutputIterator it () const { return it_; }

  append_iterator& operator= (double const sample) {
    it_ = append<Bits> (to_integral<Bits> (sample), it_);
    return *this;
  }
  append_iterator& operator* () noexcept { return *this; }
  append_iterator& operator++ () noexcept {
    it_++;
    return *this;
  }
  append_iterator operator++ (int) noexcept {
    it_++;
    return *this;
  }

private:
  OutputIterator it_;
};

/// \tparam OutputIterator  A type compatible with the requirements of an output
///   iterator.
/// \param riff_size  The size of the output file in bytes.
/// \param dest  The beginning of the destination range.
/// \returns  Output iterator to the element in the destination range, one past
///   the last element written.
template <typename OutputIterator>
OutputIterator write_header (uint32_t const riff_size, OutputIterator dest) {
  dest = append4cc ('R', 'I', 'F', 'F', dest);  // Header ckID
  dest = append<32> (riff_size, dest);          // Header ckSize
  dest = append4cc ('W', 'A', 'V', 'E', dest);  // Header contents
  return dest;
}

// The size of the RIFF chunk is drawn from the size of the following header,
// format, and data chunks. The latter includes the sample data.
constexpr auto bit_depth = uint8_t{24};
static_assert (bit_depth % 8 == 0 && bit_depth <= 32U,
               "bit depth must be a multiple of 8 and less than 33");
constexpr uint32_t format_size = sizeof (uint16_t)     // wFormatTag
                                 + sizeof (uint16_t)   // wChannels
                                 + sizeof (uint32_t)   // dwSamplesPerSec
                                 + sizeof (uint32_t)   // dwAvgBytesPerSec
                                 + sizeof (uint16_t)   // wBlockAlign
                                 + sizeof (uint16_t);  // wBitsPerSample

/// \tparam OutputIterator  A type compatible with the requirements of an output
///   iterator.
/// \param sample_rate  The sample rate in Hertz.
/// \param dest  The beginning of the destination range.
/// \returns  Output iterator to the element in the destination range, one past
///   the last element written.
template <typename OutputIterator>
OutputIterator write_format (unsigned const sample_rate, OutputIterator dest) {
  // WAVE format chunk common fields:
  // struct {
  //   uint16_t wFormatTag;       // Format category
  //   uint16_t wChannels;        // Number of channels
  //   uint32_t dwSamplesPerSec;  // Sampling rate
  //   uint32_t dwAvgBytesPerSec; // For buffer estimation
  //   uint16_t wBlockAlign;      // Data block size
  // }
  constexpr auto WAVE_FORMAT_PCM = uint16_t{0x0001};
  // WAVE format chunk header.
  dest = append4cc ('f', 'm', 't', ' ', dest);
  dest = append<32> (format_size, dest);
  // Now the WAVE format chunk common fields.
  dest = append<16> (WAVE_FORMAT_PCM, dest);  // wFormatTag (PCM audio format)
  dest = append<16> (uint16_t{1U}, dest);     // wChannels (Number of channels)
  dest = append<32> (static_cast<uint32_t> (sample_rate),
                     dest);  // dwSamplesPerSec (sample rate)
  dest = append<32> (static_cast<uint32_t> ((sample_rate * bit_depth) / 8U),
                     dest);  // dwAvgBytesPerSec (bytes per second)
  dest = append<16> (static_cast<uint16_t> (bit_depth / 8U),
                     dest);  // wBlockAlign (bytes per block)

  // PCM format specific
  // struct {
  //   uint16_t wBitsPerSample; // Sample size
  // }
  dest = append<16> (uint16_t{bit_depth}, dest);
  return dest;
}

/// \tparam ForwardIterator  A type compatible with the requirements of a
///   forward iterator and whose value-type is a floating-point type.
/// \tparam OutputIterator  A type compatible with the requirements of an output
///   iterator.
/// \param first Iterator for the first sample.
/// \param last Iterator for the end of the sample range.
/// \param data_size  The number of bytes in the chunk payload.
/// \param dest  The iterator to which data it written.
/// \return Output iterator one past the last element written.
template <typename ForwardIterator, typename OutputIterator>
OutputIterator write_data (ForwardIterator const first,
                           ForwardIterator const last, uint32_t const data_size,
                           OutputIterator dest) {
  // Data chunk header.
  dest = append4cc ('d', 'a', 't', 'a', dest);  // Data ckID
  dest = append<32> (data_size, dest);          // Data ckSize
  // Data chunk contents.
  dest =
      std::copy (first, last, append_iterator<OutputIterator, bit_depth> (dest))
          .it ();
  return dest;
}

}  // end namespace details

/// \tparam ForwardIterator  A type compatible with the requirements of a
///   forward iterator and whose value-type is a floating-point type.
/// \tparam OutputIterator  A type compatible with the requirements of an output
///   iterator.
/// \param first_sample  Iterator pointing to the first sample.
/// \param last_sample  Iterator pointing to the end of the range of samples.
/// \param sample_rate  The sample rate in Hertz.
/// \param dest  The beginning of the destination range.
/// \returns  Output iterator to the element in the destination range, one past
///   the last element written.
template <typename ForwardIterator, typename OutputIterator,
          typename = typename std::enable_if_t<std::is_floating_point_v<
              typename std::iterator_traits<ForwardIterator>::value_type>>>
OutputIterator emit_wave_file (ForwardIterator first_sample,
                               ForwardIterator last_sample,
                               unsigned const sample_rate,
                               OutputIterator dest) {
  using namespace details;

  constexpr uint32_t fourcc_size = 4U;
  constexpr uint32_t chunk =
      fourcc_size + sizeof (uint32_t);  // Size of each chunk's ckID + ckSize
  uint32_t const data_size =
      clamped_distance (first_sample, last_sample) * (bit_depth / 8U);
  uint32_t const riff_size = fourcc_size    // WAVE signature
                             + chunk        // Format chunk header
                             + format_size  // Format chunk contents
                             + chunk        // Data chunk header
                             + data_size;   // Data chunk contents
  dest = write_header (riff_size, dest);
  dest = write_format (sample_rate, dest);
  dest = write_data (first_sample, last_sample, data_size, dest);
  return dest;
}

}  // end namespace synth

#endif  // SYNTH_WAV_FILE_HPP
