//===- API.h - Offload GPU API --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_API_H
#define OFFLOADTEST_API_API_H

namespace offloadtest {

enum class GPUAPI { Unknown, DirectX, Vulkan, Metal };

}

#endif // OFFLOADTEST_API_API_H
