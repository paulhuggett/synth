if (NOT SYSTEM_IS_IOS)
  add_executable (wav_writer main.cpp wav_file.hpp)
  target_link_libraries (wav_writer PRIVATE synth)
  setup_target (wav_writer)

  set (test_target test_synth)
  set (OUT_XML "${CMAKE_BINARY_DIR}/${test_target}.xml")
  set (command_line "$<TARGET_FILE:${test_target}>" "--gtest_output=xml:${OUT_XML}")
  add_custom_command (
    TARGET wav_writer
    PRE_LINK
    COMMAND ${command_line}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Running ${test_target}"
    DEPENDS ${test_target}
    BYPRODUCTS ${OUT_XML}
    VERBATIM
  )

endif (NOT SYSTEM_IS_IOS)
