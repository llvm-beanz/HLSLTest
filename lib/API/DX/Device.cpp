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
#include "HLSLTest/API/Pipeline.h"
#include "HLSLTest/WinError.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Error.h"

#include <codecvt>
#include <locale>

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

  struct InvocationState {
    CComPtr<ID3D12RootSignature> RootSig;
    CComPtr<ID3D12DescriptorHeap> DescHeap;
    CComPtr<ID3D12PipelineState> PSO;
  };

public:
  DXDevice(CComPtr<IDXGIAdapter1> A, CComPtr<ID3D12Device> D,
           DXGI_ADAPTER_DESC1 Desc)
      : Adapter(A), Device(D) {
    Description = StringFromWString(std::wstring(Desc.Description, 128));
  }
  DXDevice(const DXDevice &) = default;

  ~DXDevice() override = default;

  llvm::StringRef getAPIName() const override { return "DirectX"; }
  GPUAPI getAPI() const override { return GPUAPI::DirectX; }

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
      queryCapabilities();
    return Caps;
  }

  void queryCapabilities() {
    CD3DX12FeatureSupport Features;
    Features.Init(Device);

#define D3D_FEATURE_BOOL(Name)                                                 \
  Caps.insert(                                                                 \
      std::make_pair(#Name, make_capability<bool>(#Name, Features.Name())));

#define D3D_FEATURE_UINT(Name)                                                 \
  Caps.insert(std::make_pair(                                                  \
      #Name, make_capability<uint32_t>(#Name, Features.Name())));

#include "DXFeatures.def"
  }

  llvm::Error createRootSignature(Pipeline &P, InvocationState &State) {
    std::vector<D3D12_ROOT_PARAMETER> RootParams;
    uint32_t DescriptorCount = P.getDescriptorCount();
    std::unique_ptr<D3D12_DESCRIPTOR_RANGE[]> Ranges =
        std::unique_ptr<D3D12_DESCRIPTOR_RANGE[]>(
            new D3D12_DESCRIPTOR_RANGE[DescriptorCount]);

    uint32_t RangeIdx = 0;
    for (const auto &D : P.Sets) {
      uint32_t DescriptorIdx = 0;
      uint32_t StartRangeIdx = RangeIdx;
      for (const auto &R : D.Resources) {
        switch (R.Access) {
        case DataAccess::ReadOnly:
          Ranges.get()[RangeIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
          break;
        case DataAccess::ReadWrite:
          Ranges.get()[RangeIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
          break;
        case DataAccess::Constant:
          Ranges.get()[RangeIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
          break;
        }
        Ranges.get()[RangeIdx].NumDescriptors = 1;
        Ranges.get()[RangeIdx].BaseShaderRegister = R.DXBinding.Register;
        Ranges.get()[RangeIdx].RegisterSpace = R.DXBinding.Space;
        Ranges.get()[RangeIdx].OffsetInDescriptorsFromTableStart =
            DescriptorIdx;
        RangeIdx++;
      }
      if (D.Resources.size() > 0)
        RootParams.push_back(
            D3D12_ROOT_PARAMETER{D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                                 {D3D12_ROOT_DESCRIPTOR_TABLE{
                                     static_cast<uint32_t>(D.Resources.size()),
                                     &Ranges[StartRangeIdx]}},
                                 D3D12_SHADER_VISIBILITY_ALL});
    }

    D3D12_ROOT_SIGNATURE_DESC Desc = D3D12_ROOT_SIGNATURE_DESC{
        static_cast<uint32_t>(RootParams.size()), RootParams.data(), 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_NONE};

    CComPtr<ID3DBlob> Signature;
    CComPtr<ID3DBlob> Error;
    if (auto Err = HR::toError(
            D3D12SerializeRootSignature(&Desc, D3D_ROOT_SIGNATURE_VERSION_1,
                                        &Signature, &Error),
            "Failed to seialize root signature.")) {
      std::string Msg =
          std::string(reinterpret_cast<char *>(Error->GetBufferPointer()),
                      Error->GetBufferSize() / sizeof(char));
      return joinErrors(
          std::move(Err),
          llvm::createStringError(std::errc::protocol_error, Msg.c_str()));
    }

    if (auto Err = HR::toError(
            Device->CreateRootSignature(0, Signature->GetBufferPointer(),
                                        Signature->GetBufferSize(),
                                        IID_PPV_ARGS(&State.RootSig)),
            "Failed to create root signature."))
      return Err;

    return llvm::Error::success();
  }

  llvm::Error createDescriptorHeap(Pipeline &P, InvocationState &State) {
    const D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, P.getDescriptorCount(),
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
    if (auto Err = HR::toError(Device->CreateDescriptorHeap(
                                   &HeapDesc, IID_PPV_ARGS(&State.DescHeap)),
                               "Failed to create descriptor heap."))
      return Err;
    Device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return llvm::Error::success();
  }

  llvm::Error createPSO(Pipeline &P, llvm::StringRef DXIL,
                        InvocationState &State) {
    const D3D12_COMPUTE_PIPELINE_STATE_DESC Desc = {
        State.RootSig,
        {DXIL.data(), DXIL.size()},
        0,
        {
            nullptr,
            0,
        },
        D3D12_PIPELINE_STATE_FLAG_NONE};
    if (auto Err = HR::toError(
            Device->CreateComputePipelineState(&Desc, IID_PPV_ARGS(&State.PSO)),
            "Failed to create PSO."))
      return Err;
    return llvm::Error::success();
  }

  llvm::Error executeProgram(llvm::StringRef Program, Pipeline &P) override {
    InvocationState State;
    llvm::outs() << "Configuring execution on device: " << Description << "\n";
    if (auto Err = createRootSignature(P, State))
      return Err;
    llvm::outs() << "RootSignature created.\n";
    if (auto Err = createDescriptorHeap(P, State))
      return Err;
    llvm::outs() << "Descriptor heap created.\n";
    if (auto Err = createPSO(P, Program, State))
      return Err;
    llvm::outs() << "PSO created.\n";
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
