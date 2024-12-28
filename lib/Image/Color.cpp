//===- Color.cpp - Color Description ----------------------------*- C++ -*-===//
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

#include <math.h>

using namespace hlsltest;

constexpr Color D65WhitePoint = Color(95.047, 100.000, 108.883, Color::XYZ);

static Color multiply(const Color LHS, double Mat[9], Color::Space NewSpace) {
  double X, Y, Z;
  X = (LHS.X * Mat[0]) + (LHS.Y * Mat[1]) + (LHS.Z * Mat[2]);
  Y = (LHS.X * Mat[3]) + (LHS.Y * Mat[4]) + (LHS.Z * Mat[5]);
  Z = (LHS.X * Mat[6]) + (LHS.Y * Mat[7]) + (LHS.Z * Mat[8]);
  return Color(X, Y, Z, NewSpace);
}

static Color RGBToXYZ(const Color Old) {
  // Matrix assumes D65 white point.
  // Source: http://brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
  double Mat[] = {0.4124564, 0.3575761, 0.1804375, 0.2126729, 0.7151522,
                  0.0721750, 0.0193339, 0.1191920, 0.9503041};
  return multiply(Old, Mat, Color::XYZ);
}

static Color XYZToRGB(const Color Old) {
  // Matrix assumes D65 white point.
  // Source: http://brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
  double Mat[] = {3.2404542, -1.5371385, -0.4985314, -0.9692660, 1.8760108,
                  0.0415560, 0.0556434,  -0.2040259, 1.0572252};
  return multiply(Old, Mat, Color::RGB);
}

static double convertXYZ(double Val) {
  constexpr double E = 216.0 / 24389.0;
  constexpr double K = 24389.0 / 27.0;
  return Val > E ? pow(Val, 1.0 / 3.0) : (K * Val + 16.0) / 116.0;
}

static Color XYZToLAB(const Color Old) {
  double X = convertXYZ(Old.X / D65WhitePoint.X);
  double Y = convertXYZ(Old.Y / D65WhitePoint.Y);
  double Z = convertXYZ(Old.Z / D65WhitePoint.Z);

  double L = fmax(0.0, 116.0 * Y - 16.0);
  double A = 500 * (X - Y);
  double B = 200 * (Y - Z);
  return Color(L, A, B, Color::LAB);
}

static double convertLAB(double Val) {
  double ValPow3 = pow(Val, 3.0);
  if (ValPow3 > 0.008856)
    return ValPow3;

  constexpr double V = 16.0 / 116.0;
  constexpr double K = (24389.0 / 27.0) / 116.0;
  return (Val - V) / K;
}

static Color LABToXYZ(const Color Old) {
  double Y = (Old.X + 16) / 116;
  double X = Old.Y / 500 + Y;
  double Z = Y - Old.Z / 200;

  X = convertLAB(X) * D65WhitePoint.X;
  Y = convertLAB(Y) * D65WhitePoint.Y;
  Z = convertLAB(Z) * D65WhitePoint.Z;

  return Color(X, Y, Z, Color::XYZ);
}

Color Color::translateSpaceImpl(Space NewCS) {
  Color Tmp = *this;
  if (ColorSpace == Color::RGB)
    Tmp = RGBToXYZ(*this);
  else if (ColorSpace == Color::LAB)
    Tmp = LABToXYZ(*this);
  // Tmp is now in XYZ space.
  if (NewCS == Color::RGB)
    return XYZToRGB(Tmp);
  if (NewCS == Color::LAB)
    return XYZToLAB(Tmp);
  return Tmp;
}
