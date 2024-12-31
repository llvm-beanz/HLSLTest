//===- gpu-exec.cpp - HLSL GPU Execution Tool -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "API/API.h"
#include "API/Device.h"
#include "Config.h"
#include "Image/Image.h"
#include "Support/Pipeline.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/ToolOutputFile.h"
#include <string>

using namespace llvm;
using namespace hlsltest;

static cl::opt<std::string>
    InputPipeline(cl::Positional, cl::desc("<input pipeline description>"),
                  cl::value_desc("filename"));

static cl::opt<std::string> InputShader(cl::Positional,
                                        cl::desc("<input compiled shader>"),
                                        cl::value_desc("filename"));

static cl::opt<GPUAPI>
    APIToUse("api", cl::desc("GPU API to use"), cl::init(GPUAPI::Unknown),
             cl::values(clEnumValN(GPUAPI::DirectX, "dx", "DirectX"),
                        clEnumValN(GPUAPI::Vulkan, "vk", "Vulkan"),
                        clEnumValN(GPUAPI::Metal, "mtl", "Metal")));

static cl::opt<std::string> OutputFilename("o", cl::desc("Output filename"),
                                           cl::value_desc("filename"),
                                           cl::init("-"));

static cl::opt<std::string>
    ImageOutput("r", cl::desc("Resource name to output as png"),
                cl::value_desc("<name>"), cl::init(""));

static cl::opt<bool>
    Quiet("quiet", cl::desc("Suppress printing the pipeline as output"));

static cl::opt<bool> UseWarp("warp", cl::desc("Use warp"));

std::unique_ptr<MemoryBuffer> readFile(const std::string &Path) {
  ExitOnError ExitOnErr("gpu-exec: error: ");
  ErrorOr<std::unique_ptr<MemoryBuffer>> FileOrErr =
      MemoryBuffer::getFileOrSTDIN(Path);
  ExitOnErr(errorCodeToError(FileOrErr.getError()));

  return std::move(FileOrErr.get());
}

int run();

int main(int ArgC, char **ArgV) {
  InitLLVM X(ArgC, ArgV);
  cl::ParseCommandLineOptions(ArgC, ArgV, "GPU Execution Tool");

  if (run()) {
    errs() << "No device available.";
    return 1;
  }
  return 0;
}

int run() {
  ExitOnError ExitOnErr("gpu-exec: error: ");
  ExitOnErr(Device::initialize());

  std::unique_ptr<MemoryBuffer> ShaderBuf = readFile(InputShader);

  // Try to guess the API by reading the shader binary.
  if (APIToUse == GPUAPI::Unknown) {
    if (ShaderBuf->getBuffer().starts_with("DXBC")) {
      APIToUse = GPUAPI::DirectX;
      outs() << "Using DirectX API\n";
    } else if (*reinterpret_cast<const uint32_t *>(
                   ShaderBuf->getBuffer().data()) == 0x07230203) {
      APIToUse = GPUAPI::Vulkan;
      outs() << "Using Vulkan API\n";
    } else if (ShaderBuf->getBuffer().starts_with("MTLB")) {
      APIToUse = GPUAPI::Metal;
      outs() << "Using Metal API\n";
    }
  }

  if (UseWarp && APIToUse != GPUAPI::DirectX)
    ExitOnErr(createStringError(std::errc::executable_format_error,
                                "WARP required DirectX API"));

  if (APIToUse == GPUAPI::Unknown)
    ExitOnErr(
        createStringError(std::errc::executable_format_error,
                          "Could not identify API to execute provided shader"));

  std::unique_ptr<MemoryBuffer> PipelineBuf = readFile(InputPipeline);
  Pipeline PipelineDesc;
  yaml::Input YIn(PipelineBuf->getBuffer());
  YIn >> PipelineDesc;
  ExitOnErr(llvm::errorCodeToError(YIn.error()));

  for (const auto &D : Device::devices()) {
    if (D->getAPI() != APIToUse)
      continue;
    if (UseWarp && D->getDescription() != "Microsoft Basic Render Driver")
      continue;
    ExitOnErr(D->executeProgram(ShaderBuf->getBuffer(), PipelineDesc));

    if (Quiet)
      return 0;

    std::error_code EC;
    llvm::sys::fs::OpenFlags OpenFlags = llvm::sys::fs::OF_None;
    if (ImageOutput.empty()) {
      OpenFlags |= llvm::sys::fs::OF_Text;
      auto Out =
          std::make_unique<llvm::ToolOutputFile>(OutputFilename, EC, OpenFlags);
      ExitOnErr(llvm::errorCodeToError(EC));

      yaml::Output YOut(Out->os());
      YOut << PipelineDesc;
      Out->keep();
      return 0;
    }
    for (const auto &S : PipelineDesc.Sets) {
      for (const auto &R : S.Resources) {
        if (R.OutputProps.Name == ImageOutput) {
          ImageRef Img = ImageRef(R);
          ExitOnErr(Image::writePNG(Img, OutputFilename));
          return 0;
        }
      }
    }

    ExitOnErr(
        createStringError(Twine("No descriptor with name ") + ImageOutput));
  }
  return 1;
}
