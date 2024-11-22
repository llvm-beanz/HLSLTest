function(try_compile_hlsl output)
  if (DEFINED ${output})
    return()
  endif()
  # NOTE: `SOURCE` needs to be a multi-argument because source code
  # often contains semicolons which happens to be CMake's list separator
  # which confuses `cmake_parse_arguments()`.
  cmake_parse_arguments(ARG "" "" "SOURCE;FLAGS" ${ARGN})
  if (ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unexpected arguments \"${ARG_UNPARSED_ARGUMENTS}\"")
  endif()
  if(NOT ARG_SOURCE)
    set(ARG_SOURCE "export int foo(int x, int y) { return x + y; }\n")
  endif()
  set(SIMPLE_HLSL ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/src.hlsl)
  file(WRITE ${SIMPLE_HLSL} "${ARG_SOURCE}\n")
  string(REGEX MATCHALL "<[A-Za-z0-9_]*>" substitutions
         ${CMAKE_C_COMPILE_OBJECT})

  set(TRY_COMPILE_FLAGS "${ARG_FLAGS}")
  string(REPLACE ";" " " extra_flags "${TRY_COMPILE_FLAGS}")

  set(test_compile_command ${HLSLTEST_COMPILER} ${extra_flags} ${SIMPLE_HLSL})
  string(REPLACE "\"" "" test_compile_command "${test_compile_command}")
  string(REPLACE " " ";" test_compile_command "${test_compile_command}")

  execute_process(
    COMMAND ${test_compile_command}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE TEST_OUTPUT
    ERROR_VARIABLE TEST_ERROR
  )
  CHECK_COMPILER_FLAG_COMMON_PATTERNS(_CheckCCompilerFlag_COMMON_PATTERNS)
  set(ERRORS_FOUND OFF)
  foreach(var ${_CheckCCompilerFlag_COMMON_PATTERNS})
    if("${var}" STREQUAL "FAIL_REGEX")
      continue()
    endif()
    if("${TEST_ERROR}" MATCHES "${var}" OR "${TEST_OUTPUT}" MATCHES "${var}")
      set(ERRORS_FOUND ON)
    endif()
  endforeach()

  if(result EQUAL 0 AND NOT ERRORS_FOUND)
    set(result True)
  else()
    file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "Testing compiler for supporting " ${ARGN} ":\n"
        "Command: ${test_compile_command}\n"
        "${TEST_OUTPUT}\n${TEST_ERROR}\n${result}\n")
    set(result False)
  endif()
  set(${output} ${result} CACHE BOOL "HLSL compiler supports SPIR-V")
  message(STATUS "${output} - ${${output}}")
endfunction()

try_compile_hlsl(SUPPORTS_SPIRV SOURCE
                "RWBuffer<int> In : register(u0);
                RWBuffer<int> Out : register(u1);

                [numthreads(8,1,1)]
                void main(uint3 TID : SV_GroupThreadID) {
                  Out[TID.x] = In[TID.x];
                }"
                FLAGS -T cs_6_0 -spirv
)


configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
  PATHS
  "HLSLTEST_BINARY_DIR"
  "LLVM_TOOLS_DIR"
  )

list(APPEND HLSLTEST_DEPS
  api-query
  gpu-exec
  FileCheck
  split-file
  clang)

add_custom_target(hlsl-test-depends DEPENDS ${HLSLTEST_DEPS})

add_lit_testsuite(check-hlsl "Running the HLSL regression tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${HLSLTEST_DEPS}
  )
set_target_properties(check-hlsl PROPERTIES FOLDER "HLSL tests")

add_lit_testsuites(CLANG ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${HLSLTEST_DEPS}
  FOLDER "HLSL tests/Suites"
)
