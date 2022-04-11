add_executable (wav_writer main.cpp wav_file.hpp)
target_link_libraries (wav_writer PRIVATE synth)
setup_target (wav_writer)
