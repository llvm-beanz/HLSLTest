# I'm having trouble getting sccache to work on windows, so for now skip it
# unless we're on Unix.
if (UNIX)
  find_program(SCCACHE sccache HINTS /usr/local/bin)
  if (SCCACHE)
    set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE} CACHE STRING "")
    set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE} CACHE STRING "")
  endif()
endif()
