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

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/InitLLVM.h"

using namespace llvm;

int main(int ArgC, char** ArgV) {
  InitLLVM X(ArgC, ArgV);
  cl::ParseCommandLineOptions(ArgC, ArgV, "GPU API Query Tool");
  return 0;
}