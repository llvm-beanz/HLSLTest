# hlsl-test-suite
Experimental Runtime test suite for HLSL

# Adding to LLVM Build

Add the following to the CMake options:

```shell
-DLLVM_EXTERNAL_HLSLTEST_SOURCE_DIR=${workspaceRoot}\..\hlsl-test-suite -DLLVM_EXTERNAL_PROJECTS="HLSLTest"
```