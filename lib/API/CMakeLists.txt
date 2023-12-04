if (HLSLTEST_ENABLE_VULKAN)
  #list(APPEND api_sources VK/Device.cpp)
endif()

if (HLSLTEST_ENABLE_D3D12)
  list(APPEND api_sources DX/Device.cpp)
endif()

add_hlsl_library(API
  Capabilities.cpp
  ${api_sources})
