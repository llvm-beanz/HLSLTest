//===- DXFeatures.h - DirectX Features ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef HLSLTEST_API_DXFEATURES_H
#define HLSLTEST_API_DXFEATURES_H

namespace hlsltest {
namespace directx {

#define SHADER_MODEL_ENUM(NewCase, OldCase, Value) NewCase = Value,
enum ShaderModel {
    #include "DXFeatures.def"
};

}

} // namespace hlsltest

#endif // HLSLTEST_API_DXFEATURES_H