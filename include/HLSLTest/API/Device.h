//===- Device.h - HLSL API Device API -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef HLSLTEST_API_DEVICE_H
#define HLSLTEST_API_DEVICE_H

namespace hlsltest {

class Capabilities;

class Device {
public:
virtual Capabilities getCapabilities() = 0;
virtual ~Device() {}
};

}

#endif // HLSLTEST_API_DEVICE_H
