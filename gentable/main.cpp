// -*- mode: c++; coding: utf-8-unix; -*-
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

int main () {
  constexpr auto bits = 32;  // number of bits
  constexpr auto mul = (1 << (bits - 2));

  auto const indent = std::string (4, ' ');
  auto const new_line = ",\n" + indent;

  std::cout << "// CORDIC in " << bits << " bit signed fixed point math\n"
            << "constexpr auto CORDIC_NTAB = " << bits << ";\n"
            << "constexpr uint32_t cordic_ctab [CORDIC_NTAB] = {\n"
            << std::setfill ('0') << std::hex;
  auto separator = indent;
  for (auto ctr = 0; ctr < bits; ++ctr) {
    auto const c = static_cast<uint32_t> (std::atan (std::pow (2, -ctr)) * mul);
    std::cout << separator << "0x" << std::setw (bits / 4) << c;
    separator = ((ctr + 1) % 4 == 0) ? new_line : ", ";
  }
  std::cout << "\n};\n";
}
