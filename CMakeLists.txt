project (synth CXX)
cmake_minimum_required (VERSION 3.16)

option (WERROR "Errors as warnings" Off)
set (CMAKE_CXX_STANDARD 17 CACHE STRING "C++ Standard Version")

if (APPLE AND ${CMAKE_SYSTEM_NAME} MATCHES "iOS")
  set (SYSTEM_IS_IOS Yes)
else()
  set (SYSTEM_IS_IOS No)
endif ()

function (setup_target target)
  set (clang_options
    -Weverything
    -Wno-c99-extensions
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-c++14-extensions
    -Wno-disabled-macro-expansion
    -Wno-exit-time-destructors
    -Wno-global-constructors
    -Wno-padded
    -Wno-weak-vtables
    -Wno-nullability-extension
  )
  set (gcc_options
    -Wall
    -Wextra
    -pedantic
  )
  set (msvc_options
    -W4
    -wd4324 # 4324: structure was padded due to alignment specifier
  )

  if (WERROR)
      list (APPEND clang_options -Werror)
      list (APPEND gcc_options -Werror)
      list (APPEND msvc_options /WX)
  endif ()

  target_compile_options (${target} PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${clang_options}>
    $<$<CXX_COMPILER_ID:GNU>:${gcc_options}>
    $<$<CXX_COMPILER_ID:MSVC>:${msvc_options}>
  )
endfunction (setup_target)

if (NOT SYSTEM_IS_IOS)
  add_subdirectory (systemc)
  add_subdirectory (cordic_sine)
  add_subdirectory (gentable)
  add_subdirectory (scsynth)
endif ()
add_subdirectory (synth)
