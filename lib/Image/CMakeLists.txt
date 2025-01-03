add_hlsl_library(Image
                 Color.cpp
                 Image.cpp
                 ImageComparators.cpp)

target_include_directories(HLSLTestImage PRIVATE SYSTEM BEFORE
                           "${HLSLTEST_BINARY_DIR}/third-party/libpng/"
                           "${HLSLTEST_SOURCE_DIR}/third-party/libpng/")
target_link_libraries(HLSLTestImage INTERFACE png_static)
add_dependencies(HLSLTestImage png_genfiles)
