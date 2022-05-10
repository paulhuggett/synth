add_executable (test_synth )
target_sources (test_synth PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/test_oscillator.cpp")
target_link_libraries(test_synth PRIVATE gmock_main synth)
