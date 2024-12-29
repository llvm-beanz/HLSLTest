//===- Color.h - Color Description ------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef HLSLTEST_IMAGE_COLOR_H
#define HLSLTEST_IMAGE_COLOR_H

namespace hlsltest {

class Color {
public:
  enum Space {
    RGB,
    XYZ,
    LAB
  };

  double R, G, B;

  Space ColorSpace;

  constexpr Color(double Rx, double Gy, double Bz, Space CS = RGB)
      : R(Rx), G(Gy), B(Bz), ColorSpace(CS) {}
  Color() = delete;
  Color(const Color &) = default;
  Color(Color &&) = default;
  Color &operator=(const Color &) = default;
  Color &operator=(Color &&) = default;

  Color translateSpace(Space NewCS) {
    if (NewCS == ColorSpace)
      return *this;
    return translateSpaceImpl(NewCS);
  }

private:
  Color translateSpaceImpl(Space NewCS);
};

} // namespace hlsltest

#endif // HLSLTEST_IMAGE_COLOR_H
