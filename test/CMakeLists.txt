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

list(APPEND HLSLTEST_DEPS
  api-query
  gpu-exec
  FileCheck
  split-file
  clang)

if (HLSLTEST_TEST_CLANG)
  list(APPEND HLSLTEST_DEPS clang)
endif()

add_custom_target(hlsl-test-depends DEPENDS ${HLSLTEST_DEPS})

try_compile_hlsl(SUPPORTS_SPIRV SOURCE
                "RWBuffer<int> In : register(u0);
                RWBuffer<int> Out : register(u1);

                [numthreads(8,1,1)]
                void main(uint3 TID : SV_GroupThreadID) {
                  Out[TID.x] = In[TID.x];
                }"
                FLAGS -T cs_6_0 -spirv
)

# Initialize all platforms to false.
set(TEST_d3d12 False)
set(TEST_vk False)
set(TEST_mtl False)
set(FORCE_CLANG False)

if (HLSLTEST_ENABLE_D3D12)
  list(APPEND platforms_to_test d3d12)
endif()

if (SUPPORTS_SPIRV AND HLSLTEST_ENABLE_VULKAN)
  list(APPEND platforms_to_test vk)
endif()

if (HLSLTEST_ENABLE_METAL)
  list(APPEND platforms_to_test mtl)
endif()

llvm_canonicalize_cmake_booleans(HLSLTEST_TEST_CLANG)

foreach(platform ${platforms_to_test})
  set(TEST_${platform} False)
endforeach()

umbrella_lit_testsuite_begin(check-hlsl)
foreach(platform ${platforms_to_test})
  set(TEST_${platform} True)
  set(suite ${platform})

  configure_lit_site_cfg(
    ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/${suite}/lit.site.cfg.py
    MAIN_CONFIG
    ${CMAKE_CURRENT_BINARY_DIR}/${suite}/lit.cfg.py
    PATHS
    "HLSLTEST_BINARY_DIR"
    "LLVM_TOOLS_DIR"
    )

    add_lit_testsuite(check-hlsl-${suite}
    "Running the HLSL regression tests for ${suite}"
    ${CMAKE_CURRENT_BINARY_DIR}/${suite}
    DEPENDS ${HLSLTEST_DEPS}
    )
  set_target_properties(check-hlsl-${suite} PROPERTIES FOLDER "HLSL tests")

  add_lit_testsuites(hlsl-${suite} ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${HLSLTEST_DEPS}
    FOLDER "HLSL tests/Suites/${suite}"
  )

  if (HLSLTEST_TEST_CLANG)
    set(FORCE_CLANG True)
    set(suite clang-${platform})

    configure_lit_site_cfg(
      ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
      ${CMAKE_CURRENT_BINARY_DIR}/${suite}/lit.site.cfg.py
      MAIN_CONFIG
      ${CMAKE_CURRENT_BINARY_DIR}/${suite}/lit.cfg.py
      PATHS
      "HLSLTEST_BINARY_DIR"
      "LLVM_TOOLS_DIR"
      )

      add_lit_testsuite(check-hlsl-${suite}
      "Running the HLSL regression tests for ${suite}"
      ${CMAKE_CURRENT_BINARY_DIR}/${suite}
      DEPENDS ${HLSLTEST_DEPS}
      )
    set_target_properties(check-hlsl-${suite} PROPERTIES FOLDER "HLSL tests")

    add_lit_testsuites(hlsl-${suite} ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS ${HLSLTEST_DEPS}
      FOLDER "HLSL tests/Suites/${suite}"
    )
    set(FORCE_CLANG False)
  endif()

  set(TEST_${platform} False)
endforeach()

umbrella_lit_testsuite_end(check-hlsl)
set_target_properties(check-hlsl PROPERTIES FOLDER "HLSL tests")
