# This project is only buildable as a subproject of LLVM.

project(HLSLTest)

# Add path for custom modules
list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  )

set(HLSLTEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include
                    ${CMAKE_CURRENT_BINARY_DIR}/include)

macro(add_hlsl_tool name)
  add_llvm_executable(${name} ${ARGN})
  set_target_properties(${name} PROPERTIES FOLDER "HLSL Test/Tools")
endmacro()

macro(add_hlsl_library name)
  add_llvm_library(HLSLTest${name} ${ARGN})
  set_target_properties(HLSLTest${name} PROPERTIES FOLDER "HLSL Test/Libraries")
endmacro()

find_package(Vulkan)
message(STATUS "Vulkan Include Dirs: ${Vulkan_INCLUDE_DIRS}")
if (Vulkan_INCLUDE_DIRS)
  set(HLSLTEST_ENABLE_VULKAN On)
endif ()

find_package(D3D12)
message(STATUS "D3D12 Include Dirs: ${D3D12_INCLUDE_DIRS}")
if (D3D12_INCLUDE_DIRS)
  set(HLSLTEST_ENABLE_D3D12 On)
endif ()

if (NOT HLSLTEST_ENABLE_VULKAN AND NOT HLSLTEST_ENABLE_D3D12)
  message(FATAL_ERROR "No supported runtime API")
endif ()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/HLSLTest/Config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/HLSLTest/Config.h)

if (D3D12_INCLUDE_DIRS AND NOT DIRECTX_HEADERS_PATH)
  set(DIRECTX_HEADERS_PATH ${CURRENT_CMAKE_BINARY_DIR}/DirectX-Headers)
  ExternalProject_Add(DIRECTX_HEADERS
                      GIT_REPOSITORY https://github.com/microsoft/DirectX-Headers.git
                      SOURCE_DIR ${DIRECTX_HEADERS_PATH})
endif ()

add_subdirectory(lib)
add_subdirectory(tools)
