//===- api-query.cpp - HLSL API Query Tool --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "HLSLTest/API/Capabilities.h"
#include "HLSLTest/API/Device.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/InitLLVM.h"


using namespace llvm;
using namespace hlsltest;

int main(int ArgC, char **ArgV) {
  InitLLVM X(ArgC, ArgV);
  cl::ParseCommandLineOptions(ArgC, ArgV, "GPU API Query Tool");

  ExitOnError ExitOnErr("api-query: error: ");

  ExitOnErr(Device::initialize());

  for (const auto &D : Device::devices()) {
    for (const auto &C : D->getCapabilities()) {
      C.second.print(llvm::outs());
      llvm::outs() << "\n";
    }
  }
  return 0;
}