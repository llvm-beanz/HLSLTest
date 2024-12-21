//===- API.h - HLSL GPU API -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef HLSLTEST_API_API_H
#define HLSLTEST_API_API_H

namespace hlsltest {

enum class GPUAPI { Unknown, DirectX, Vulkan, Metal };

}

#endif // HLSLTEST_API_API_H
