//===- Pipeline.h - GPU Pipeline Description --------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//
//===----------------------------------------------------------------------===//

#ifndef HLSLTEST_SUPPORT_PIPELINE_H
#define HLSLTEST_SUPPORT_PIPELINE_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/YAMLTraits.h"
#include <memory>
#include <string>

namespace hlsltest {

enum class DataFormat {
  Hex8,
  Hex16,
  Hex32,
  Hex64,
  UInt16,
  UInt32,
  UInt64,
  Int16,
  Int32,
  Int64,
  Float32,
  Float64,
};

enum class DataAccess {
  ReadOnly,
  ReadWrite,
  Constant,
};

struct DirectXBinding {
  uint32_t Register;
  uint32_t Space;
};

struct OutputProperties {
  std::string Name;
  int Height;
  int Width;
  int Depth;
};

struct Resource {
  DataFormat Format;
  int Channels;
  int RawSize;
  DataAccess Access;
  size_t Size;
  std::unique_ptr<char[]> Data;
  DirectXBinding DXBinding;
  OutputProperties OutputProps;

  bool isRaw() const {
    return RawSize > 0;
  }

  uint32_t getElementSize() const {
    if (isRaw())
      return RawSize;
    return getSingleElementSize() * Channels;
  }

  uint32_t getSingleElementSize() const {
    switch (Format) {
    case DataFormat::Hex8:
      return 1;
    case DataFormat::Hex16:
    case DataFormat::UInt16:
    case DataFormat::Int16:
      return 2;
    case DataFormat::Hex32:
    case DataFormat::UInt32:
    case DataFormat::Int32:
    case DataFormat::Float32:
      return 4;
    case DataFormat::Hex64:
    case DataFormat::UInt64:
    case DataFormat::Int64:
    case DataFormat::Float64:
      return 8;
    }
    llvm_unreachable("All cases covered.");
  }
};

struct DescriptorSet {
  llvm::SmallVector<Resource> Resources;
};

struct Pipeline {
  int DispatchSize[3];
  llvm::SmallVector<DescriptorSet> Sets;

  uint32_t getDescriptorCount() const {
    uint32_t DescriptorCount = 0;
    for (auto &D : Sets)
      DescriptorCount += D.Resources.size();
    return DescriptorCount;
  }
};
} // namespace hlsltest

LLVM_YAML_IS_SEQUENCE_VECTOR(hlsltest::DescriptorSet)
LLVM_YAML_IS_SEQUENCE_VECTOR(hlsltest::Resource)

namespace llvm {
namespace yaml {

template <> struct MappingTraits<hlsltest::Pipeline> {
  static void mapping(IO &I, hlsltest::Pipeline &P);
};

template <> struct MappingTraits<hlsltest::DescriptorSet> {
  static void mapping(IO &I, hlsltest::DescriptorSet &D);
};

template <> struct MappingTraits<hlsltest::Resource> {
  static void mapping(IO &I, hlsltest::Resource &R);
};

template <> struct MappingTraits<hlsltest::DirectXBinding> {
  static void mapping(IO &I, hlsltest::DirectXBinding &B);
};

template <> struct MappingTraits<hlsltest::OutputProperties> {
  static void mapping(IO &I, hlsltest::OutputProperties &P);
};

template <> struct ScalarEnumerationTraits<hlsltest::DataFormat> {
  static void enumeration(IO &I, hlsltest::DataFormat &V) {
#define ENUM_CASE(Val) I.enumCase(V, #Val, hlsltest::DataFormat::Val)
    ENUM_CASE(Hex8);
    ENUM_CASE(Hex16);
    ENUM_CASE(Hex32);
    ENUM_CASE(Hex64);
    ENUM_CASE(UInt16);
    ENUM_CASE(UInt32);
    ENUM_CASE(UInt64);
    ENUM_CASE(Int16);
    ENUM_CASE(Int32);
    ENUM_CASE(Int64);
    ENUM_CASE(Float32);
    ENUM_CASE(Float64);
#undef ENUM_CASE
  }
};

template <> struct ScalarEnumerationTraits<hlsltest::DataAccess> {
  static void enumeration(IO &I, hlsltest::DataAccess &V) {
#define ENUM_CASE(Val) I.enumCase(V, #Val, hlsltest::DataAccess::Val)
    ENUM_CASE(ReadOnly);
    ENUM_CASE(ReadWrite);
    ENUM_CASE(Constant);
#undef ENUM_CASE
  }
};
} // namespace yaml
} // namespace llvm

#endif // HLSLTEST_SUPPORT_PIPELINE_H
