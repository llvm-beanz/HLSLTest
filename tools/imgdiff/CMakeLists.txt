add_hlsl_tool(imgdiff
              imgdiff.cpp)

target_link_libraries(imgdiff PRIVATE
                      LLVMSupport
                      HLSLTestImage
                      HLSLTestSupport)
