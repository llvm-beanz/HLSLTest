//===- Image.h - Image Description ------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef HLSLTEST_IMAGE_IMAGE_H
#define HLSLTEST_IMAGE_IMAGE_H

#include "Support/Pipeline.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/bit.h"
#include "llvm/Support/Error.h"

#include <cassert>
#include <cstdint>

namespace hlsltest {

class ImageRef {
  uint32_t Height;
  uint32_t Width;
  uint8_t Depth;
  uint8_t Channels;
  bool IsFloat;

protected:
  llvm::StringRef Data;
  ImageRef(uint32_t H, uint32_t W, uint8_t D, uint8_t C, bool F)
      : Height(H), Width(W), Depth(D), Channels(C), IsFloat(F), Data() {}

public:
  ImageRef() = default;
  ImageRef(const ImageRef &I) = default;
  ImageRef(ImageRef &&II) = default;
  ImageRef &operator=(const ImageRef &) = default;
  ImageRef &operator=(ImageRef &&) = default;

  ImageRef(uint32_t H, uint32_t W, uint8_t D, uint8_t C, bool F,
           llvm::StringRef S)
      : Height(H), Width(W), Depth(D), Channels(C), IsFloat(F), Data(S) {
    assert((Channels == 3 || Channels == 4) && "Channels must be 3 or 4");
    assert(llvm::popcount(Depth) == 1 && "Depth must be a power of 2");
    assert(Depth <= 8 && "Depth must be <= 8 (64-bit)");
    assert(Data.size() == size() && "Data size does not match properties");
  }

  ImageRef(const Resource &R)
      : ImageRef(R.OutputProps.Height, R.OutputProps.Width,
                 R.getSingleElementSize(), R.Channels,
                 R.Format == DataFormat::Float32 ||
                     R.Format == DataFormat::Float64,
                 llvm::StringRef(R.Data.get(), R.Size)) {}

  uint32_t getHeight() const { return Height; }
  uint32_t getWidth() const { return Width; }
  uint8_t getDepth() const { return Depth; }
  uint32_t getBitDepth() const { return Depth * 8; }
  uint8_t getChannels() const { return Channels; }
  bool isFloat() const { return IsFloat; }

  size_t size() const { return Data.size(); }
  const char *data() const { return Data.data(); }
  bool empty() const { return Data.empty(); }
};

class Image : public ImageRef {
  std::unique_ptr<char[]> OwnedData;

  Image(uint32_t H, uint32_t W, uint8_t D, uint8_t C, bool F)
      : ImageRef(H, W, D, C, F) {
    uint64_t Sz = static_cast<uint64_t>(H) * static_cast<uint64_t>(W) *
                  static_cast<uint64_t>(D) * static_cast<uint64_t>(C);
    OwnedData = std::make_unique<char[]>(Sz);
    Data = llvm::StringRef(OwnedData.get(), Sz);
  }

public:
  // Not default constructable.
  Image() = delete;
  // Not copyable.
  Image(const Image &I) = delete;
  Image &operator=(const Image &I) = delete;
  // Movable.
  Image(Image &&I) = default;
  Image &operator=(Image &&I) = default;

  ImageRef getRef() const { return ImageRef(*this); }

  static llvm::Error WritePNG(ImageRef I, llvm::StringRef Path);

  static Image translateImage(ImageRef I, uint8_t Depth, uint8_t Channels,
                              bool Float);

  char *data() { return OwnedData.get(); }
};

} // namespace hlsltest

#endif // HLSLTEST_IMAGE_IMAGE_H
