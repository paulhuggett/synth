set (SYNTH_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../include")

add_library (synth STATIC
  "${SYNTH_INCLUDES}/synth/envelope.hpp"
  "${SYNTH_INCLUDES}/synth/fixed.hpp"
  "${SYNTH_INCLUDES}/synth/nco.hpp"
  "${SYNTH_INCLUDES}/synth/uint.hpp"
  "${SYNTH_INCLUDES}/synth/voice.hpp"
  "${SYNTH_INCLUDES}/synth/wavetable.hpp"

  envelope.cpp
)
target_include_directories (synth PUBLIC "${SYNTH_INCLUDES}")
setup_target (synth)
