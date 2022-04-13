// -*- mode: c++; coding: utf-8-unix; -*-
#include "synth/wavetable.hpp"

namespace synth {

wavetable const sine{[] (double const theta) { return std::sin (theta); }};

wavetable const square{
    [] (double const theta) { return theta <= pi ? 1.0 : -1.0; }};

wavetable const triangle{[] (double const theta) {
  return (theta <= pi ? theta : (two_pi - theta)) / half_pi - 1.0;
}};

wavetable const sawtooth{[] (double const theta) { return theta / pi - 1.0; }};

}  // end namespace synth
