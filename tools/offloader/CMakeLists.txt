add_hlsl_tool(offloader
              offloader.cpp
              WritePNG.cpp)

target_include_directories(offloader PRIVATE SYSTEM BEFORE "${HLSLTEST_BINARY_DIR}/third-party/libpng/")

target_link_libraries(offloader PRIVATE LLVMSupport HLSLTestAPI png_static)
