//===- Pipeline.cpp - HLSL API Pipeline API -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "HLSLTest/API/Pipeline.h"

using namespace hlsltest;

namespace llvm {
namespace yaml {
void MappingTraits<hlsltest::Pipeline>::mapping(IO &I, hlsltest::Pipeline &P) {
  I.mapRequired("DescriptorSets", P.Sets);
}

void MappingTraits<hlsltest::DescriptorSet>::mapping(
    IO &I, hlsltest::DescriptorSet &D) {
  I.mapRequired("Resources", D.Resources);
}

void MappingTraits<hlsltest::Resource>::mapping(IO &I, hlsltest::Resource &R) {
  I.mapRequired("Format", R.Format);
  switch (R.Format) {
#define DATA_CASE(Enum, Type)                                                  \
  case DataFormat::Enum: {                                                     \
    if (I.outputting()) {                                                      \
      llvm::MutableArrayRef<Type> Arr(reinterpret_cast<Type *>(R.Data.get()),  \
                                      R.Size / sizeof(Type));                  \
      I.mapRequired("Data", Arr);                                              \
    } else {                                                                   \
      llvm::SmallVector<Type, 64> Arr;                                         \
      I.mapRequired("Data", Arr);                                              \
      R.Size = Arr.size() * sizeof(Type);                                      \
      R.Data.reset(new char[R.Size]);                                          \
      memcpy(R.Data.get(), Arr.data(), R.Size);                                \
    }                                                                          \
    break;                                                                     \
  }
    DATA_CASE(Hex8, llvm::yaml::Hex8)
    DATA_CASE(Hex16, llvm::yaml::Hex16)
    DATA_CASE(Hex32, llvm::yaml::Hex32)
    DATA_CASE(Hex64, llvm::yaml::Hex64)
    DATA_CASE(UInt16, uint16_t)
    DATA_CASE(UInt32, uint32_t)
    DATA_CASE(UInt64, uint64_t)
    DATA_CASE(Int16, int16_t)
    DATA_CASE(Int32, int32_t)
    DATA_CASE(Int64, int64_t)
    DATA_CASE(Float32, float)
    DATA_CASE(Float64, double)
  }
}
} // namespace yaml
} // namespace llvm
