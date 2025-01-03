//===- ImageComparators.cpp - Image Comparators -----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "Image/ImageComparators.h"

using namespace llvm;
using namespace hlsltest;

void yaml::MappingTraits<CompareCheck>::mapping(IO &I, CompareCheck &C) {
  I.mapRequired("Type", C.Type);
  if (C.Type == CompareCheck::Intervals)
    I.mapRequired("Vals", C.Vals);
  else
    I.mapRequired("Val", C.Val);
}
