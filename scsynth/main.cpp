#include "scnco.hpp"

using namespace scsynth;

int sc_main (int argc, char* argv[]) {
  (void)argc;
  (void)argv;
  sc_signal<bool> clock;
  sc_signal<bool> reset;
  sc_signal<frequency> f;
  sc_signal<amplitude> out;

  oscillator osc{"osc"};
  osc.clock (clock);
  osc.reset (reset);
  osc.f (f);
  osc.out (out);

  sc_start ();

  if (!sc_end_of_simulation_invoked ()) {
    std::cout << "Simulation stopped without explicit sc_stop()\n";
    sc_stop ();
  }
  return 0;
}
