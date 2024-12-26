//===- Image.cpp - Image Description ----------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "Image/Image.h"

#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/SwapByteOrder.h"

#include <limits.h>
#include <png.h>
#include <stdio.h>

using namespace hlsltest;

template <typename DstType, typename SrcType>
void TranslatePixelData(Image &Dst, ImageRef Src) {
  uint64_t Pixels = Dst.getHeight() * Dst.getWidth();
  uint32_t CopiedChannels = std::min(Dst.getChannels(), Src.getChannels());
  DstType *DstPtr = reinterpret_cast<DstType *>(Dst.data());
  const SrcType *SrcPtr = reinterpret_cast<const SrcType *>(Src.data());
  for (uint64_t I = 0; I < Pixels; ++I) {
    for (uint32_t J = 0; J < CopiedChannels; ++J, ++SrcPtr, ++DstPtr) {
      double Tmp = static_cast<double>(*SrcPtr);
      // If the source type is not a float, normalize it between 0 & 1.
      if constexpr (!std::is_floating_point<SrcType>())
        Tmp /= std::numeric_limits<SrcType>::max();
      // If the destination type is not a float, scale it into the integer type.
      if constexpr (!std::is_floating_point<DstType>())
        Tmp *= std::numeric_limits<DstType>::max();

      *DstPtr = static_cast<DstType>(Tmp);
      if constexpr (sizeof(DstType) > 1 && !llvm::sys::IsBigEndianHost)
        llvm::sys::swapByteOrder(*DstPtr);
    }
    // If the destination has more channels fill it with saturated values.
    for (uint32_t J = 0; J < CopiedChannels - Dst.getChannels();
         ++J, ++DstPtr) {
      if constexpr (std::is_floating_point<DstType>())
        *DstPtr = 1.0;
      else
        *DstPtr = std::numeric_limits<DstType>::max();
    }
    // If the source has more channels skip them.
    for (uint32_t J = 0; J < CopiedChannels - Src.getChannels(); ++J, ++SrcPtr)
      ;
  }
}

template <typename SrcType> void translatePixelSrc(Image &Dst, ImageRef Src) {

  switch (Dst.getDepth()) {
  case 1:
    assert(!Dst.isFloat() && "No float8 support!");
    TranslatePixelData<uint8_t, SrcType>(Dst, Src);
    break;
  case 2:
    assert(!Dst.isFloat() && "No float16 support!");
    TranslatePixelData<uint16_t, SrcType>(Dst, Src);
    break;
  default:
    llvm_unreachable("Destination depth out of expected range.");
  }
}

void translatePixels(Image &Dst, ImageRef Src) {
  switch (Src.getDepth()) {
  case 1:
    assert(!Src.isFloat() && "No float8 support!");
    translatePixelSrc<uint8_t>(Dst, Src);
    break;
  case 2:
    assert(!Src.isFloat() && "No float16 support!");
    translatePixelSrc<uint16_t>(Dst, Src);
    break;
  case 4:
    if (Src.isFloat())
      translatePixelSrc<float>(Dst, Src);
    else
      translatePixelSrc<uint32_t>(Dst, Src);
    break;
  case 8:
    if (Src.isFloat())
      translatePixelSrc<double>(Dst, Src);
    else
      translatePixelSrc<uint64_t>(Dst, Src);
    break;
  default:
    llvm_unreachable("Source depth out of expected range.");
  }
}

Image Image::translateImage(ImageRef Src, uint8_t Depth, uint8_t Channels,
                            bool Float) {
  Image NewImage =
      Image(Src.getHeight(), Src.getWidth(), Depth, Channels, Float);
  translatePixels(NewImage, Src);
  return NewImage;
}

llvm::Error WritePNGImpl(ImageRef Img, llvm::StringRef OutputPath) {
  png_structp PNG =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!PNG)
    return llvm::createStringError(std::errc::io_error,
                                   "Failed creating PNG data");

  png_infop PNGInfo = png_create_info_struct(PNG);
  FILE *F = nullptr;
  auto ScopeExit = llvm::make_scope_exit([&PNG, &PNGInfo, &F]() {
    png_destroy_write_struct(&PNG, &PNGInfo);
    if (F)
      fclose(F);
  });
  if (!PNGInfo)
    return llvm::createStringError(std::errc::io_error,
                                   "Failed writing PNG info");

  F = fopen(OutputPath.data(), "wb");
  if (!F)
    return llvm::createStringError(std::errc::io_error, "Failed openiong file");
  png_init_io(PNG, F);
  png_set_IHDR(PNG, PNGInfo, Img.getWidth(), Img.getHeight(), Img.getBitDepth(),
               PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_colorp ColorP =
      (png_colorp)png_malloc(PNG, PNG_MAX_PALETTE_LENGTH * sizeof(png_color));
  if (!ColorP)
    return llvm::createStringError(std::errc::io_error,
                                   "Failed creating color palette");
  png_set_PLTE(PNG, PNGInfo, ColorP, PNG_MAX_PALETTE_LENGTH);
  png_write_info(PNG, PNGInfo);
  png_set_packing(PNG);

  png_bytepp Rows =
      (png_bytepp)png_malloc(PNG, Img.getHeight() * sizeof(png_bytep));
  uint64_t RowSize = Img.getWidth() * Img.getChannels() * Img.getDepth();
  // Step one row back from the end
  const uint8_t *Row = reinterpret_cast<const uint8_t *>(Img.data()) +
                       (RowSize * Img.getHeight()) - RowSize;
  for (uint32_t I = 0; I < Img.getHeight(); ++I, Row -= RowSize)
    Rows[I] = const_cast<png_bytep>(Row);

  png_write_image(PNG, Rows);
  png_write_end(PNG, PNGInfo);
  png_free(PNG, ColorP);
  return llvm::Error::success();
}

llvm::Error Image::WritePNG(ImageRef Img, llvm::StringRef Path) {
  uint32_t NewDepth = std::min(static_cast<uint32_t>(Img.getDepth()), 2u);
  if (Img.isFloat() || Img.getDepth() != NewDepth) {
    Image Translated = translateImage(Img, NewDepth, Img.getChannels(), false);
    return WritePNGImpl(Translated, Path);
  }
  return WritePNGImpl(Img, Path);
}
