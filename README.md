# HLSLTest
Experimental Runtime test suite for HLSL

# Adding to LLVM Build

Add the following to the CMake options:

```shell
-DLLVM_EXTERNAL_HLSLTEST_SOURCE_DIR=${workspaceRoot}\..\HLSLTest -DLLVM_EXTERNAL_PROJECTS="HLSLTest"
```

# YAML Pipeline Format

This framework provides a YAML representation for describing GPU pipelines and buffers. The format is implemented by the `API/Pipeline.{h|cpp}` sources. The following is an example pipleine YAML description:

```yaml
---
DescriptorSets:
  - Resources:
    - Access: Constant
      Format: Int32
      Data: [ 1, 2, 3, 4, 5, 6, 7, 8]
      DirectXBinding:
        Register: 0 # implies b0 due to Access being RW
        Space: 0
    - Access: ReadOnly
      Format: Float32
      Data: [ 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8]
      DirectXBinding:
        Register: 0 # implies t0 due to Access being RO
        Space: 0
  - Resources:
    - Access: ReadOnly
      Format: Hex16
      Data: [ 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8]
      DirectXBinding:
        Register: 1 # implies t1 due to Access being RO
        Space: 0
...
```
