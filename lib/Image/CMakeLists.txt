add_offloadtest_library(Image
                 Color.cpp
                 Image.cpp
                 ImageComparators.cpp)

target_include_directories(OffloadTestImage PRIVATE SYSTEM BEFORE
                           "${OFFLOADTEST_BINARY_DIR}/third-party/libpng/"
                           "${OFFLOADTEST_SOURCE_DIR}/third-party/libpng/")
target_link_libraries(OffloadTestImage INTERFACE png_static)
add_dependencies(OffloadTestImage png_genfiles)
