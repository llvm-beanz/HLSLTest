//===- imgdiff.cpp - Image Comparison Tool --------------------------------===//
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

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/InitLLVM.h"
#include <string>

using namespace llvm;
using namespace hlsltest;

static cl::opt<std::string> ExpectedPath(cl::Positional,
                                         cl::desc("<expected image>"),
                                         cl::value_desc("filename"));

static cl::opt<std::string> ActualPath(cl::Positional,
                                       cl::desc("<actual image>"),
                                       cl::value_desc("filename"));

enum class ComparisonMode { ExactMatch, CIE76 };

static cl::opt<ComparisonMode>
    Mode("mode", cl::desc("Comparison Mode"), cl::init(ComparisonMode::CIE76),
         cl::values(clEnumValN(ComparisonMode::ExactMatch, "exact",
                               "Exact Match of decoded image data"),
                    clEnumValN(ComparisonMode::CIE76, "cie76",
                               "CIE76 Color Difference")));

static cl::opt<std::string> OutputFilename("o", cl::desc("Output filename"),
                                           cl::value_desc("filename"));

int main(int ArgC, char **ArgV) {
  InitLLVM X(ArgC, ArgV);
  cl::ParseCommandLineOptions(ArgC, ArgV, "Image Comparison Tool");
  ExitOnError ExitOnErr("imgdiff: error: ");

  Image ExpectedImage = ExitOnErr(Image::loadPNG(ExpectedPath));
  Image ActualImage = ExitOnErr(Image::loadPNG(ActualPath));
  if (ExpectedImage.size() != ActualImage.size())
    ExitOnErr(createStringError(std::errc::executable_format_error,
                                "Image sizes do not match"));
  switch (Mode) {
  case ComparisonMode::ExactMatch:
    if (0 !=
        memcmp(ExpectedImage.data(), ActualImage.data(), ExpectedImage.size()))
      ExitOnErr(createStringError(std::errc::executable_format_error,
                                  "Images do not match"));
    return 0;
  case ComparisonMode::CIE76: {
    double RMS = 0.0;
    double Furthest = 0.0;
    auto ExDiff =
        Image::computeDistance(ExpectedImage, ActualImage, RMS, Furthest);
    if (!ExDiff)
      ExitOnErr(ExDiff.takeError());
    llvm::outs() << "RMS Difference: " << RMS << "\n";
    llvm::outs() << "Furthest Pixel Difference: " << Furthest << "\n";
    if (!OutputFilename.empty())
      ExitOnErr(Image::writePNG(*ExDiff, OutputFilename));

    // 2.3 corresponds to a "just noticeable distance" for the CIE76
    // difference algorithm. If no pixels are worse than that, there should be
    // no noticeable difference in the image.
    if (Furthest < 2.3)
      return 0;
  }
  }

  return 1;
}
