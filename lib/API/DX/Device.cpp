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
#include <d3dx12.h>
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

#include <locale>
#include <codecvt>

using namespace hlsltest;
namespace {

std::string StringFromWString(const std::wstring &In) {
  using convert_type = std::codecvt_utf8<wchar_t>;
  std::wstring_convert<convert_type, wchar_t> Converter;
  return Converter.to_bytes(In);
}

class DXDevice : public hlsltest::Device {
private:
  CComPtr<IDXGIAdapter1> Adapter;
  CComPtr<ID3D12Device> Device;
  Capabilities Caps;

public:
  DXDevice(CComPtr<IDXGIAdapter1> A, CComPtr<ID3D12Device> D,
           DXGI_ADAPTER_DESC1 Desc)
      : Adapter(A), Device(D) {
        Description = StringFromWString(std::wstring(Desc.Description, 128));
      }
  DXDevice(const DXDevice &) = default;

  ~DXDevice() override = default;

  llvm::StringRef getAPIName() const override { return "DirectX"; }

  static llvm::Expected<DXDevice> Create(CComPtr<IDXGIAdapter1> Adapter) {
    CComPtr<ID3D12Device> Device;
    if (auto Err =
            HR::toError(D3D12CreateDevice(Adapter, D3D_FEATURE_LEVEL_11_0,
                                          IID_PPV_ARGS(&Device)),
                        "Failed to create D3D device"))
      return Err;
    DXGI_ADAPTER_DESC1 Desc;
    if (auto Err = HR::toError(Adapter->GetDesc1(&Desc),
                               "Failed to get device description"))
      return Err;
    return DXDevice(Adapter, Device, Desc);
  }

  const Capabilities &getCapabilities() override {
    if (Caps.empty())
      llvm::cantFail(queryCapabilities()); // assert on error!
    return Caps;
  }

  llvm::Error queryCapabilities() {
    CD3DX12FeatureSupport Features;
    Features.Init(Device);

#define D3D_FEATURE_BOOL(Name, Desc)                                           \
  Caps.insert(                                                                 \
      std::make_pair(#Name, make_capability<bool>(#Name, Features.Name())));

#define D3D_FEATURE_UINT(Name, Desc)                                           \
  Caps.insert(std::make_pair(                                                  \
      #Name, make_capability<uint32_t>(#Name, Features.Name())));

#include "DXFeatures.def"
    return llvm::Error::success();
  }
};

class DirectXContext {
private:
  CComPtr<IDXGIFactory2> Factory;
  llvm::SmallVector<std::shared_ptr<DXDevice>> Devices;

  DirectXContext() = default;
  ~DirectXContext() = default;

public:
  llvm::Error initialize() {
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
      auto ShPtr = std::make_shared<DXDevice>(*ExDevice);
      Devices.push_back(ShPtr);
      Device::registerDevice(std::static_pointer_cast<Device>(ShPtr));
    }
    return llvm::Error::success();
  }

  using iterator = llvm::SmallVector<std::shared_ptr<DXDevice>>::iterator;

  iterator begin() { return Devices.begin(); }
  iterator end() { return Devices.end(); }

  static DirectXContext &Instance() {
    static DirectXContext Ctx;
    return Ctx;
  }
};
} // namespace

llvm::Error InitializeDXDevices() {
  return DirectXContext::Instance().initialize();
}
