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

#include "HLSLTest/API/Capabilities.h"
#include "llvm/ADT/iterator_range.h"

#include <memory>

namespace hlsltest {

class Device {
public:
virtual const Capabilities &getCapabilities() = 0;
virtual ~Device() = 0;

static void registerDevice(std::shared_ptr<Device> D);
static llvm::Error initialize();

using DeviceArray = llvm::SmallVector<std::shared_ptr<Device>>;
using DeviceIterator = DeviceArray::iterator;
using DeviceRange = llvm::iterator_range<DeviceIterator>;

static DeviceIterator begin();
static DeviceIterator end();
static inline DeviceRange devices() {
  return DeviceRange(begin(), end());
}

};

}

#endif // HLSLTEST_API_DEVICE_H
