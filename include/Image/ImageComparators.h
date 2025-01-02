//===- ImageComparators.h - Image Comparators -------------------*- C++ -*-===//
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
#include "Image/Image.h"

#include "llvm/Support/raw_ostream.h"

#include <algorithm>

#ifndef HLSLTEST_IMAGE_IMAGECOMPARATOR_H
#define HLSLTEST_IMAGE_IMAGECOMPARATOR_H

namespace hlsltest {
class ImageComparatorDistance : public ImageComparatorBase {
  double Furthest = 0.0;
  double RMS = 0.0;
  uint64_t Count = 0;

public:
  void processPixel(Color L, Color R) override {
    double Distance = Color::CIE75Distance(L, R);
    Furthest = std::max(Furthest, Distance);
    RMS += Distance;
    Count += 1;
  }

  void print(llvm::raw_ostream &OS) override {
    OS << "RMS Difference: " << RMS << "\n";
    OS << "Furthest Pixel Difference: " << Furthest << "\n";
  }

  bool result() override {
    RMS /= (static_cast<double>(Count) * 3.0);
    // 2.3 corresponds to a "just noticeable distance" for the CIE76
    // difference algorithm. If no pixels are worse than that, there should be
    // no noticeable difference in the image.
    return Furthest < 2.3;
  }
};

class ImageComparatorDiffImage : public ImageComparatorBase {
  Image DiffImg;
  float *DiffPtr;
  llvm::StringRef OutputFilename;

public:
  virtual ~ImageComparatorDiffImage() {}
  ImageComparatorDiffImage(uint32_t Height, uint32_t Width, llvm::StringRef OF)
      : DiffImg(Height, Width, 4, 3, true), OutputFilename(OF) {
    DiffPtr = reinterpret_cast<float *>(DiffImg.data());
  }
  void processPixel(Color L, Color R) override {
    // TODO: I should probably instead use a color distance for this too...
    DiffPtr[0] = abs(L.R - R.R);
    DiffPtr[1] = abs(L.G - R.G);
    DiffPtr[2] = abs(L.B - R.B);
    DiffPtr += 3;
  }

  void print(llvm::raw_ostream &) override {
    llvm::consumeError(Image::writePNG(DiffImg, OutputFilename));
  }
};
} // namespace hlsltest

#endif // HLSLTEST_IMAGE_IMAGECOMPARATOR_H
