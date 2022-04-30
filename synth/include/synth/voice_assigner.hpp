// -*- mode: c++; coding: utf-8-unix; -*-
#ifndef SYNTH_VOICE_ASSIGNER_HPP
#define SYNTH_VOICE_ASSIGNER_HPP

#include <array>
#include <limits>

#include "synth/voice.hpp"

namespace synth {

class voice_assigner {
public:
  voice_assigner () = default;

  void note_on (unsigned note);
  void note_off (unsigned note);

  double tick ();

  void set_wavetable (wavetable const *w);
  void set_envelope (envelope::phase stage, double value);

private:
  static constexpr auto unassigned = std::numeric_limits<unsigned>::max ();
  struct vm {
    vm ();
    voice v;
    unsigned note;
  };
  std::array<vm, 8> voices_;
  unsigned next_ = 0U;
};

}  // end namespace synth

#endif  // SYNTH_VOICE_ASSIGNER_HPP
