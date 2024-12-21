//===- WritePNG.cpp - Utility for writing PNG -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "Support/Pipeline.h"

#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/SwapByteOrder.h"

#include <limits.h>
#include <png.h>
#include <stdio.h>

using namespace hlsltest;

template <typename DstType, typename SrcType>
void TranslatePixelData(DstType *Dst, const SrcType *Src, uint64_t Count) {
  for (uint64_t I = 0; I < Count; ++I, ++Src, ++Dst) {
    SrcType Val = *Src;
    Val *= std::numeric_limits<DstType>::max();
    *Dst = static_cast<DstType>(Val);
    if (sizeof(DstType) > 1 && !llvm::sys::IsBigEndianHost)
      llvm::sys::swapByteOrder(*Dst);
  }
}

llvm::Error WritePNG(llvm::StringRef OutputPath, const Resource &R) {
  int Height = R.OutputProps.Height;
  int Width = R.OutputProps.Width;
  int Depth = R.OutputProps.Depth;
  assert(Depth == 16 || Depth == 8 && "Color depth should be 8 or 16 bits.");
  // Translate pixel data into uint8_t.
  uint64_t Stride = R.getSingleElementSize();
  uint64_t PixelComponents = R.Size / Stride;
  std::unique_ptr<uint8_t[]> TranslatedPixels;
  bool PixelsNeedTranslation =
      R.Format == DataFormat::Float32 || R.Format == DataFormat::Float64;
  if (PixelsNeedTranslation)
    TranslatedPixels =
        std::make_unique<uint8_t[]>(PixelComponents * (Depth / 8));

  switch (R.Format) {
  case DataFormat::Float32:
    if (Depth == 16)
      TranslatePixelData(reinterpret_cast<uint16_t *>(TranslatedPixels.get()),
                         reinterpret_cast<const float *>(R.Data.get()),
                         PixelComponents);
    else
      TranslatePixelData(reinterpret_cast<uint8_t *>(TranslatedPixels.get()),
                         reinterpret_cast<const float *>(R.Data.get()),
                         PixelComponents);
    break;
  case DataFormat::Float64:
    if (Depth == 16)
      TranslatePixelData(reinterpret_cast<uint16_t *>(TranslatedPixels.get()),
                         reinterpret_cast<const double *>(R.Data.get()),
                         PixelComponents);
    else
      TranslatePixelData(reinterpret_cast<uint8_t *>(TranslatedPixels.get()),
                         reinterpret_cast<const double *>(R.Data.get()),
                         PixelComponents);
    break;
  default:
    llvm_unreachable("Unsupported format for png output");
  }

  uint8_t *PixelData = PixelsNeedTranslation
                           ? TranslatedPixels.get()
                           : reinterpret_cast<uint8_t *>(R.Data.get());

  png_structp PNG =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL); // 8
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
  png_set_IHDR(PNG, PNGInfo, Width, Height, Depth, PNG_COLOR_TYPE_RGB_ALPHA,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
               PNG_FILTER_TYPE_BASE);
  png_colorp ColorP =
      (png_colorp)png_malloc(PNG, PNG_MAX_PALETTE_LENGTH * sizeof(png_color));
  if (!ColorP)
    return llvm::createStringError(std::errc::io_error,
                                   "Failed creating color palette");
  png_set_PLTE(PNG, PNGInfo, ColorP, PNG_MAX_PALETTE_LENGTH);
  png_write_info(PNG, PNGInfo);
  png_set_packing(PNG);

  png_bytepp Rows = (png_bytepp)png_malloc(PNG, Height * sizeof(png_bytep));
  uint64_t DepthBytes = Depth / 8;
  uint64_t RowSize = Width * R.Channels * DepthBytes;
  // Step one row back from the end
  uint8_t *Row = PixelData + (PixelComponents * DepthBytes) - RowSize;
  for (int I = 0; I < Height; ++I, Row -= RowSize)
    Rows[I] = (png_bytep)Row;

  png_write_image(PNG, Rows);
  png_write_end(PNG, PNGInfo);
  png_free(PNG, ColorP);

  return llvm::Error::success();
}
