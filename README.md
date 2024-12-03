# HLSLTest
Experimental Runtime test suite for HLSL

## Current Status

| Testing Machine | DXC | Clang |
|-----------------|-----|-------|
| Windows DirectX12 Intel GPU | ![DXC](https://github.com/llvm-beanz/HLSLTEst/actions/workflows/windows-intel-dxc-d3d12.yaml/badge.svg) | ![Clang](https://github.com/llvm-beanz/HLSLTEst/actions/workflows/windows-intel-clang-d3d12.yaml/badge.svg) |
| Windows DirectX12 Warp | ![DXC](https://github.com/llvm-beanz/HLSLTEst/actions/workflows/windows-intel-dxc-warp-d3d12.yaml/badge.svg) | ![Clang](https://github.com/llvm-beanz/HLSLTEst/actions/workflows/windows-intel-clang-warp-d3d12.yaml/badge.svg) |
| Windows Vulkan Intel GPU | ![DXC](https://github.com/llvm-beanz/HLSLTEst/actions/workflows/windows-intel-dxc-vk.yaml/badge.svg) | ![Clang](https://github.com/llvm-beanz/HLSLTEst/actions/workflows/windows-intel-clang-vk.yaml/badge.svg) |
| Windows DirectX12 NVIDIA GPU | ![DXC](https://github.com/llvm-beanz/HLSLTEst/actions/workflows/windows-nv-dxc-d3d12.yaml/badge.svg) | ![Clang](https://github.com/llvm-beanz/HLSLTEst/actions/workflows/windows-nv-clang-d3d12.yaml/badge.svg) |
| Windows Vulkan NVIDIA GPU | ![DXC](https://github.com/llvm-beanz/HLSLTEst/actions/workflows/windows-nv-dxc-vk.yaml/badge.svg) | ![Clang](https://github.com/llvm-beanz/HLSLTEst/actions/workflows/windows-nv-clang-vk.yaml/badge.svg) |
| macOS Apple M1 | ![DXC](https://github.com/llvm-beanz/HLSLTEst/actions/workflows/macos-dxc-mtl.yaml/badge.svg) | ![Clang & DXC](https://github.com/llvm-beanz/HLSLTEst/actions/workflows/macos-clang-mtl.yaml/badge.svg) |


# Prerequisites

This project requires being able to locally build LLVM and leverages LLVM's build infrastructure. It also requires installing the `pyyaml` Python package. You can install `pyyaml` by running:

```shell
pip3 install pyyaml
```

# Adding to LLVM Build

Add the following to the CMake options:

```shell
-DLLVM_EXTERNAL_HLSLTEST_SOURCE_DIR=${workspaceRoot}\..\HLSLTest -DLLVM_EXTERNAL_PROJECTS="HLSLTest"
```

If you do not have a build of dxc on your path you'll need to specify the shader
compiler to use by passing:

```shell
-DDXC_DIR=<path to folder containing dxc & dxv>
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
        Register: 0 # implies b0 due to Access being Constant
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
