if (NOT (${CMAKE_SYSTEM_NAME} STREQUAL "iOS"))

  add_executable (scsynth
    main.cpp
    scnco.hpp
    scwavetable.hpp
  )
  setup_target (scsynth)

  set (clang_options
    -Wno-suggest-override
    -Wno-suggest-destructor-override
    -Wno-zero-as-null-pointer-constant
    -Wno-old-style-cast
  )
  set (gcc_options )
  set (msvc_options )

  target_compile_options (scsynth PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${clang_options}>
    $<$<CXX_COMPILER_ID:GNU>:${gcc_options}>
    $<$<CXX_COMPILER_ID:MSVC>:${msvc_options}>
  )


  target_include_directories (scsynth PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../systemc/src"
  )
  target_compile_definitions (scsynth PRIVATE SC_INCLUDE_FX)
  target_link_libraries (scsynth PRIVATE systemc)

endif ()
