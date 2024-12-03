add_hlsl_tool(gpu-exec
              gpu-exec.cpp
              WritePNG.cpp)

target_include_directories(gpu-exec PRIVATE SYSTEM BEFORE "${HLSLTEST_BINARY_DIR}/third-party/libpng/")

target_link_libraries(gpu-exec PRIVATE LLVMSupport HLSLTestAPI png_static)