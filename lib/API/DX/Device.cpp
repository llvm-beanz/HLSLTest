//===- DX/Device.cpp - HLSL API DirectX Device API ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include <atlbase.h>
#include <d3d12.h>
#include <dxgi1_4.h>

// The windows headers define these macros which conflict with the C++ standard
// library. Undefining them before including any LLVM C++ code prevents errors.
#undef max
#undef min

#include "HLSLTest/API/Capabilities.h"
#include "HLSLTest/API/Device.h"
#include "HLSLTest/WinError.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Error.h"

namespace {

class DXDevice : hlsltest::Device {
private:
  CComPtr<ID3D12Device> Device;

public:
  DXDevice(CComPtr<ID3D12Device> D) : Device(D) {}
  DXDevice(const DXDevice &) = default;

  ~DXDevice() override = default;

  static llvm::Expected<DXDevice> Create(CComPtr<IDXGIAdapter1> Adapter) {
    CComPtr<ID3D12Device> Device;
    if (auto Err =
            HR::toError(D3D12CreateDevice(Adapter, D3D_FEATURE_LEVEL_11_0,
                                          IID_PPV_ARGS(&Device)),
                        "Failed to create D3D device"))
      return Err;
    return DXDevice(Device);
  }

  hlsltest::Capabilities getCapabilities() override {
    return hlsltest::Capabilities();
  }
};

class DirectXContext {
private:
  CComPtr<IDXGIFactory2> Factory;
  llvm::SmallVector<DXDevice> Devices;

  DirectXContext() = default;
  ~DirectXContext() = default;

  llvm::Error Initialize() {
    if (auto Err = HR::toError(CreateDXGIFactory2(0, IID_PPV_ARGS(&Factory)),
                               "Failed to create DXGI Factory")) {
      return Err;
    }
    CComPtr<IDXGIAdapter1> Adapter;
    unsigned AdapterIndex = 0;
    while (SUCCEEDED(Factory->EnumAdapters1(AdapterIndex++, &Adapter))) {
      auto ExDevice = DXDevice::Create(Adapter);
      if (!ExDevice)
        return ExDevice.takeError();
      Devices.push_back(*ExDevice);
    }
    return llvm::Error::success();
  }

public:
  static DirectXContext &Instance() {
    static DirectXContext Ctx;
    return Ctx;
  }
};
} // namespace
