#include "synth/fixed.hpp"

#include <limits>

static_assert (synth::mask<0>::value == 0);
static_assert (synth::mask<1>::value == 0b1);
static_assert (synth::mask<2>::value == 0b11);
static_assert (synth::mask<3>::value == 0b111);
static_assert (synth::mask<4>::value == 0xf);
static_assert (synth::mask<7>::value == 0x7f);
static_assert (synth::mask<8>::value == 0xff);
static_assert (synth::mask<9>::value == 0x1ff);
static_assert (synth::mask<15>::value == 0x7fff);
static_assert (synth::mask<16>::value == 0xffff);
static_assert (synth::mask<17>::value == 0x1ffff);
static_assert (synth::mask<31>::value == 0x7fffffff);
static_assert (synth::mask<32>::value == 0xffffffff);
static_assert (synth::mask<33>::value == 0x1ffffffff);
static_assert (synth::mask<63>::value == 0x7fffffffffffffff);
static_assert (synth::mask<64>::value == std::numeric_limits<uint64_t>::max());
