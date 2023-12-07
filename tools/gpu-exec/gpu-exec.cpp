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

#include "HLSLTest/API/API.h"
#include "HLSLTest/API/Device.h"
#include "HLSLTest/API/Pipeline.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
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
                        clEnumValN(GPUAPI::Vulkan, "vk", "Vulkan")));

std::unique_ptr<MemoryBuffer> readFile(const std::string &Path) {
  ExitOnError ExitOnErr("gpu-exec: error: ");
  ErrorOr<std::unique_ptr<MemoryBuffer>> FileOrErr =
      MemoryBuffer::getFileOrSTDIN(Path);
  ExitOnErr(errorCodeToError(FileOrErr.getError()));

  return std::move(FileOrErr.get());
}

int main(int ArgC, char **ArgV) {
  InitLLVM X(ArgC, ArgV);
  cl::ParseCommandLineOptions(ArgC, ArgV, "GPU API Query Tool");

  ExitOnError ExitOnErr("gpu-exec: error: ");
  ExitOnErr(Device::initialize());

  std::unique_ptr<MemoryBuffer> ShaderBuf = readFile(InputShader);

  // Try to guess the API by reading the shader binary.
  if (APIToUse == GPUAPI::Unknown) {
    if (ShaderBuf->getBuffer().startswith("DXBC"))
      APIToUse = GPUAPI::DirectX;
    if (*reinterpret_cast<const uint32_t *>(ShaderBuf->getBuffer().data()) ==
        0x07230203)
      APIToUse = GPUAPI::Vulkan;
  }

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
    ExitOnErr(D->executeProgram(ShaderBuf->getBuffer(), PipelineDesc));
    return 0;
  }
  errs() << "No device available.";
  return 1;
}
