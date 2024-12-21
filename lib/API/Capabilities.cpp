//===- Capabilities.cpp - HLSL API Capabilities API -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "API/Capabilities.h"

using namespace hlsltest;

char CapabilityValueBase::ID = 0;
char CapabilityValueBool::ID = 0;
char CapabilityValueUnsigned::ID = 0;

void CapabilityValueBase::anchor() {}
