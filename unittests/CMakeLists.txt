add_custom_target(HLSLTestUnit)

function(add_hlsltest_unittest test_dirname)
  add_unittest(HLSLTestUnit ${test_dirname} ${ARGN})
endfunction()

add_subdirectory(Image)
