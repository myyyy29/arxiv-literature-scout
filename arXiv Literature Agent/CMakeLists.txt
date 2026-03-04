
add_executable (CMakeTarget "../src/arXiv Literature Agent.cpp" "../include/arXiv Literature Agent.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET CMakeTarget PROPERTY CXX_STANDARD 20)
endif()
