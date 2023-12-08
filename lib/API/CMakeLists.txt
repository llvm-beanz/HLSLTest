if (HLSLTEST_ENABLE_VULKAN)
  list(APPEND api_sources VK/Device.cpp)
  list(APPEND api_libraries ${Vulkan_LIBRARIES})
endif()

if (HLSLTEST_ENABLE_D3D12)
  list(APPEND api_sources DX/Device.cpp DX/DXFeatures.cpp)
  list(APPEND api_libraries ${D3D12_LIBRARIES})
endif()

add_hlsl_library(API
  Capabilities.cpp
  Device.cpp
  Pipeline.cpp
  ${api_sources})

target_include_directories(HLSLTestAPI SYSTEM BEFORE
  PRIVATE ${DIRECTX_HEADERS_PATH}/include/directx
  PRIVATE ${Vulkan_INCLUDE_DIRS})

target_link_libraries(HLSLTestAPI INTERFACE ${api_libraries})
if (TARGET DIRECTX_HEADERS)
  add_dependencies(HLSLTestAPI DIRECTX_HEADERS)
endif()
