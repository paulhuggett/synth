add_executable (test_synth )
target_sources (test_synth PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/test_fixed.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_oscillator.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_wavetable.cpp"
)

set (COVERAGE_ENABLED Yes)
if (COVERAGE_ENABLED)
  set (clang_compiler_coverage -fprofile-instr-generate -fcoverage-mapping)
  set (clang_linker_coverage -fprofile-instr-generate)
  set (gcc_compiler_coverage )
  set (gcc_linker_coverage )
  set (msvc_compiler_coverage )
  set (msvc_linker_coverage )

  target_compile_options (test_synth PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${clang_compiler_coverage}>
    $<$<CXX_COMPILER_ID:GNU>:${gcc_compiler_coverage}>
    $<$<CXX_COMPILER_ID:MSVC>:${msvc_compiler_coverage}>
  )
  target_link_options (test_synth PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${clang_linker_coverage}>
    $<$<CXX_COMPILER_ID:GNU>:${gcc_linker_coverage}>
    $<$<CXX_COMPILER_ID:MSVC>:${msvc_linker_coverage}>
  )
endif (COVERAGE_ENABLED)

target_link_libraries(test_synth PRIVATE gmock_main synth)
