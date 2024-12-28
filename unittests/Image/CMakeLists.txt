add_hlsltest_unittest(ImageTests ColorTests.cpp)

target_link_libraries(ImageTests PRIVATE HLSLTestImage)
