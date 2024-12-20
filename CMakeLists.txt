# This project is only buildable as a subproject of LLVM.

project(HLSLTest)

# Add path for custom modules
list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  )

set(HLSLTEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(HLSLTEST_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(HLSLTEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(DXC_DIR "" CACHE STRING "Path to a DXC build or install binary directory")
find_program(DXC_EXECUTABLE dxc HINTS ${DXC_DIR})
find_program(DXV_EXECUTABLE dxv HINTS ${DXC_DIR})

if ("clang" IN_LIST LLVM_ENABLE_PROJECTS)
  set(default_HLSLTEST_TEST_CLANG On)
else()
  set(default_HLSLTEST_TEST_CLANG Off)
endif()
option(HLSLTEST_TEST_CLANG "Enable testing clang as the HLSL Compiler" ${default_HLSLTEST_TEST_CLANG})

option(HLSLTEST_WARP_ONLY "Only generate Warp configurations (useful for testing in VMs, Windows-only)." OFF)
if (HLSLTEST_WARP_ONLY AND NOT WIN32)
  message(FATAL_ERROR "HLSLTEST_WARP_ONLY is only suppoted on Windows hosts!")
endif()

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

if (APPLE)
  set(METAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third-party/metal-cpp
                         ${CMAKE_CURRENT_SOURCE_DIR}/third-party/metal_irconverter_runtime)
  set(HLSLTEST_ENABLE_METAL On)
endif ()

if (NOT HLSLTEST_ENABLE_VULKAN AND NOT HLSLTEST_ENABLE_D3D12 AND NOT APPLE)
  message(FATAL_ERROR "No supported runtime API")
endif ()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/Config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/Config.h)

set(DIRECTX_HEADERS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third-party/DirectX-Headers)

if (WIN32)
  message(STATUS "Including vendored zlib")
  add_subdirectory(third-party/zlib)
  # These are some extra fun hacks becauze ZLIB's CMake is fragile
  if (NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
  endif ()
  # libpng does some crazy stuff with zlib's include directories.
  set(ZLIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third-party/zlib
                        ${CMAKE_CURRENT_BINARY_DIR}/third-party/zlib CACHE STRING "" FORCE)
  set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/zlib
                        ${CMAKE_CURRENT_BINARY_DIR}/third-party/zlib CACHE STRING "" FORCE)
  include_directories(${ZLIB_INCLIDE_DIR})
  # Skip exports for libpng because they don't work with building my own zlib.
  set(SKIP_INSTALL_EXPORT On)
endif ()
add_subdirectory(third-party/libpng)

include(Warp)

add_subdirectory(lib)
add_subdirectory(tools)

add_subdirectory(test)
