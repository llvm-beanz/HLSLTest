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
  FileCheck)

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
