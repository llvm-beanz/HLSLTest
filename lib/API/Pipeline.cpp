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

namespace llvm {
namespace yaml {
void MappingTraits<hlsltest::Pipeline>::mapping(IO &I,
                                                hlsltest::Pipeline &P) {
  I.mapRequired("DescriptorSets", P.Sets);
}

void MappingTraits<hlsltest::DescriptorSet>::mapping(IO &I,
                                                hlsltest::DescriptorSet &D) {
  I.mapRequired("Resources", D.Resources);
}

void MappingTraits<hlsltest::Resource>::mapping(IO &I,
                                                hlsltest::Resource &R) {
  I.mapRequired("Format", R.Format);
}
} // namespace yaml
} // namespace llvm