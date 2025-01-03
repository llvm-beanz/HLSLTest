add_custom_target(OffloadTestUnit)

function(add_offloadtest_unittest test_dirname)
  add_unittest(OffloadTestUnit ${test_dirname} ${ARGN})
endfunction()

add_subdirectory(Image)
