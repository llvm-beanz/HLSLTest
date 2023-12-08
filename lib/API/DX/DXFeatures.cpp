//===- DX/DXFeatures.cpp - HLSL API DirectX Device API --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "DXFeatures.h"
#include "llvm/Support/ScopedPrinter.h"

using namespace hlsltest;
using namespace hlsltest::directx;
using namespace llvm;

#define SHADER_MODEL_ENUM(NewCase, Str, Value) {#Str, NewCase},
static const EnumEntry<directx::ShaderModel> ShaderModelNames[] {
#include "DXFeatures.def"
};

static ArrayRef<EnumEntry<ShaderModel>> getShaderModels() {
  return ArrayRef(ShaderModelNames);
}

#define ROOT_SIGNATURE_ENUM(NewCase, Str, Value) {#Str, NewCase},
static const EnumEntry<directx::RootSignature> RootSignatureNames[] {
#include "DXFeatures.def"
};

static ArrayRef<EnumEntry<RootSignature>> getRootSignatures() {
  return ArrayRef(RootSignatureNames);
}

namespace {
template <typename T>
std::string enumEntryToString(ArrayRef<EnumEntry<T>> EnumValues, T Value) {
  for (const EnumEntry<T> &I : EnumValues)
    if (I.Value == Value)
      return I.Name.str();
}
} // namespace

std::string CapabilityPrinter::toString(ShaderModel M) {
  return enumEntryToString(getShaderModels(), M);
}
std::string CapabilityPrinter::toString(RootSignature R) {
  return enumEntryToString(getRootSignatures(), R);
}
