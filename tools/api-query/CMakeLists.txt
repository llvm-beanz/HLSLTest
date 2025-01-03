add_offloadtest_tool(api-query
              api-query.cpp)

target_link_libraries(api-query PRIVATE LLVMSupport OffloadTestAPI)
