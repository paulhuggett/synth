if (APPLE)
add_executable (player_macos
  audio_queue.h
  audio_queue.mm
  error.h
  error.mm
  main2.mm
)
setup_target (player_macos)
target_link_libraries (player_macos PRIVATE synth "-framework Foundation" "-framework AudioToolbox")
endif (APPLE)
