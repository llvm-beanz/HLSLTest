//===- ColorTests.cpp - Color Tests -----------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "Image/Color.h"

#include "llvm/Support/raw_ostream.h"

#include "gtest/gtest.h"

#include <algorithm>

using namespace hlsltest;

template <typename IntegerTy, typename FloatTy>
IntegerTy NormalizeToInteger(FloatTy &Val) {
  constexpr FloatTy Base =
      static_cast<FloatTy>(std::numeric_limits<IntegerTy>::max()) + 1.0;
  FloatTy Conv = std::clamp(floor(Val * Base), static_cast<FloatTy>(0.0),
                            static_cast<FloatTy>(255.0));
  return static_cast<IntegerTy>(Conv);
}

TEST(ColorTests, RoundTrip) {
  {
    Color RGB = Color(1.0, 0.5, 0.0);
    Color XYZ = RGB.translateSpace(Color::XYZ);
    Color LAB = RGB.translateSpace(Color::LAB);

    Color XYZPrime = LAB.translateSpace(Color::XYZ);
    Color RGBPrime = LAB.translateSpace(Color::RGB);
    const int ULPS = 1000;

    // The floating point rounding errors across these transformations are
    // significant, so rather than comparing the float values we normalize them
    // back into integer color values to compare.
    EXPECT_EQ(NormalizeToInteger<uint16_t>(XYZ.X),
              NormalizeToInteger<uint16_t>(XYZPrime.X));
    EXPECT_EQ(NormalizeToInteger<uint16_t>(XYZ.Y),
              NormalizeToInteger<uint16_t>(XYZPrime.Y));
    EXPECT_EQ(NormalizeToInteger<uint16_t>(XYZ.Z),
              NormalizeToInteger<uint16_t>(XYZPrime.Z));

    EXPECT_EQ(NormalizeToInteger<uint16_t>(RGB.X),
              NormalizeToInteger<uint16_t>(RGBPrime.X));
    EXPECT_EQ(NormalizeToInteger<uint16_t>(RGB.Y),
              NormalizeToInteger<uint16_t>(RGBPrime.Y));
    EXPECT_EQ(NormalizeToInteger<uint16_t>(RGB.Z),
              NormalizeToInteger<uint16_t>(RGBPrime.Z));
  }

  {
    Color RGB = Color(0.125, 0.896, 0.652);
    Color XYZ = RGB.translateSpace(Color::XYZ);
    Color LAB = RGB.translateSpace(Color::LAB);

    Color XYZPrime = LAB.translateSpace(Color::XYZ);
    Color RGBPrime = LAB.translateSpace(Color::RGB);
    const int ULPS = 1000;

    // The floating point rounding errors across these transformations are
    // significant, so rather than comparing the float values we normalize them
    // back into integer color values to compare.
    EXPECT_EQ(NormalizeToInteger<uint16_t>(XYZ.X),
              NormalizeToInteger<uint16_t>(XYZPrime.X));
    EXPECT_EQ(NormalizeToInteger<uint16_t>(XYZ.Y),
              NormalizeToInteger<uint16_t>(XYZPrime.Y));
    EXPECT_EQ(NormalizeToInteger<uint16_t>(XYZ.Z),
              NormalizeToInteger<uint16_t>(XYZPrime.Z));

    EXPECT_EQ(NormalizeToInteger<uint16_t>(RGB.X),
              NormalizeToInteger<uint16_t>(RGBPrime.X));
    EXPECT_EQ(NormalizeToInteger<uint16_t>(RGB.Y),
              NormalizeToInteger<uint16_t>(RGBPrime.Y));
    EXPECT_EQ(NormalizeToInteger<uint16_t>(RGB.Z),
              NormalizeToInteger<uint16_t>(RGBPrime.Z));
  }
}
