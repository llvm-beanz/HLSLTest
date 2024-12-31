# -*- Python -*-

# Configuration file for the 'lit' test runner.

import os
import sys
import re
import platform
import subprocess
import yaml

import lit.util
import lit.formats
from lit.llvm import llvm_config
from lit.llvm.subst import FindTool
from lit.llvm.subst import ToolSubst

# name: The name of this test suite.
config.name = "HLSLTest-" + config.hlsltest_suite

# testFormat: The test format to use to interpret tests.
config.test_format = lit.formats.ShTest(not llvm_config.use_lit_shell)

# suffixes: A list of file extensions to treat as test files. This is overriden
# by individual lit.local.cfg files in the test subdirectories.
config.suffixes = [".test", ".yaml"]

# excludes: A list of directories to exclude from the testsuite. The 'Inputs'
# subdirectories contain auxiliary inputs for various tests in their parent
# directories.
config.excludes = ["Inputs", "CMakeLists.txt", "README.txt", "LICENSE.txt"]

# test_source_root: The root path where tests are located.
config.test_source_root = os.path.dirname(__file__)

# test_exec_root: The root path where tests should be run.
config.test_exec_root = os.path.join(config.hlsltest_obj_root, "test", config.hlsltest_suite)

tools = [
    ToolSubst("FileCheck", FindTool("FileCheck")),
    ToolSubst("split-file", FindTool("split-file")),
    ToolSubst("imgdiff", FindTool("imgdiff"))
]

if config.hlsltest_test_warp:
  config.available_features.add("DirectX-WARP")
  tools.append(ToolSubst("%offloader", command=FindTool("offloader"), extra_args=["-warp"]))
else:
  tools.append(ToolSubst("%offloader", FindTool("offloader")))

if config.hlsltest_test_clang:
  if os.path.exists(config.hlsltest_dxc_dir):
    tools.append(ToolSubst("dxc", FindTool("clang-dxc"), extra_args=["--dxv-path=%s" % config.hlsltest_dxc_dir]))
  else:
    tools.append(ToolSubst("dxc", FindTool("clang-dxc")))
  config.available_features.add("Clang")
else:
  tools.append(ToolSubst("dxc", config.hlsltest_dxc))

llvm_config.add_tool_substitutions(tools, config.llvm_tools_dir)

api_query = os.path.join(config.llvm_tools_dir, "api-query")
query_string = subprocess.check_output(api_query)
devices = yaml.safe_load(query_string)

for device in devices['Devices']:
  if device['API'] == "DirectX" and config.hlsltest_enable_d3d12:
    config.available_features.add("DirectX")
    if "Intel" in device['Description']:
      config.available_features.add("DirectX-Intel")
  if device['API'] == "Metal" and config.hlsltest_enable_metal:
    config.available_features.add("Metal")
  if device['API'] == "Vulkan" and config.hlsltest_enable_vulkan:
    config.available_features.add("Vulkan")
    if "NVIDIA" in device['Description']:
      config.available_features.add("Vulkan-NV")

if os.path.exists(config.goldenimage_dir):
  config.substitutions.append(("%goldenimage_dir", config.goldenimage_dir))
  config.available_features.add("goldenimage")
