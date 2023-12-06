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

#include "HLSLTest/API/API.h"
#include "HLSLTest/API/Capabilities.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/ADT/StringRef.h"

#include <memory>
#include <string>

namespace hlsltest {

struct Pipeline;

class Device {
protected:
  std::string Description;

public:
  virtual const Capabilities &getCapabilities() = 0;
  virtual llvm::StringRef getAPIName() const = 0;
  virtual GPUAPI getAPI() const = 0;
  virtual llvm::Error executePipeline(Pipeline &P) = 0;

  virtual ~Device() = 0;

  llvm::StringRef getDescription() const { return Description; }

  static void registerDevice(std::shared_ptr<Device> D);
  static llvm::Error initialize();

  using DeviceArray = llvm::SmallVector<std::shared_ptr<Device>>;
  using DeviceIterator = DeviceArray::iterator;
  using DeviceRange = llvm::iterator_range<DeviceIterator>;

  static DeviceIterator begin();
  static DeviceIterator end();
  static inline DeviceRange devices() { return DeviceRange(begin(), end()); }
};

} // namespace hlsltest

#endif // HLSLTEST_API_DEVICE_H
