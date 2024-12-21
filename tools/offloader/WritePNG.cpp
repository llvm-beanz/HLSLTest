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

#include "API/Pipeline.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"

#include <png.h>
#include <stdio.h>

using namespace hlsltest;

llvm::Error WritePNG(llvm::StringRef OutputPath, const Resource &R) {
  // TODO: Fixme
  int Height = 4096;
  int Width = 4096;
  // Translate pixel data into uint8_t.
  uint64_t Stride = R.getSingleElementSize();
  uint64_t PixelComponents = R.Size / Stride;
  std::unique_ptr<uint8_t[]> PixelData =
      std::make_unique<uint8_t[]>(PixelComponents);

  const uint8_t *Src = reinterpret_cast<const uint8_t *>(R.Data.get());
  switch (R.Format) {
  case DataFormat::Float32:
    for (uint64_t I = 0; I < PixelComponents; ++I, Src += Stride) {
      float Val = *reinterpret_cast<const float *>(Src);
      Val *= 255.f;
      PixelData[I] = static_cast<uint8_t>(Val);
    }
    break;
  default:
    llvm_unreachable("Unsupported format for png output");
  }

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
  png_set_IHDR(PNG, PNGInfo, Width, Height, 8, PNG_COLOR_TYPE_RGB_ALPHA,
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
  uint64_t RowSize = Width * R.Channels;
  // Step one row back from the end
  uint8_t *Row = PixelData.get() + PixelComponents - RowSize;
  for (int I = 0; I < Height; ++I, Row -= RowSize)
    Rows[I] = (png_bytep)Row;

  png_write_image(PNG, Rows);
  png_write_end(PNG, PNGInfo);
  png_free(PNG, ColorP);

  return llvm::Error::success();
}
