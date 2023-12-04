//===- HLSLTest/WinError.h - Windows Error Utils ----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/Error.h"

namespace HR {
inline llvm::Error toError(HRESULT HR, llvm::StringRef Msg) {
  if (FAILED(HR)) {
    std::error_code EC =
        std::error_code(static_cast<int>(HR), std::system_category());
    return llvm::createStringError(EC, Msg);
  }
  return llvm::Error::success();
}
} // namespace HR
