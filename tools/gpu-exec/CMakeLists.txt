add_hlsl_tool(gpu-exec
              gpu-exec.cpp)

target_link_libraries(gpu-exec PRIVATE LLVMSupport HLSLTestAPI)
