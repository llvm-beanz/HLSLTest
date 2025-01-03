add_offloadtest_tool(imgdiff
              imgdiff.cpp)

target_link_libraries(imgdiff PRIVATE
                      LLVMSupport
                      OffloadTestImage
                      OffloadTestSupport)
