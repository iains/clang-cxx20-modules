//=== Serialization/PCHContainerOperations.cpp - PCH Containers -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file defines PCHContainerOperations and RawPCHContainerOperation.
//
//===----------------------------------------------------------------------===//

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Serialization/PCHContainerOperations.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Lex/ModuleLoader.h"
#include "llvm/Bitstream/BitstreamReader.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include <utility>

using namespace clang;

PCHContainerWriter::~PCHContainerWriter() {}
PCHContainerReader::~PCHContainerReader() {}

namespace {

/// A PCHContainerGenerator that writes out the PCH to a flat file.
class RawPCHContainerGenerator : public ASTConsumer {
  std::shared_ptr<PCHBuffer> Buffer;
  std::unique_ptr<raw_pwrite_stream> OS;

public:
  RawPCHContainerGenerator(std::unique_ptr<llvm::raw_pwrite_stream> OS,
                           std::shared_ptr<PCHBuffer> Buffer)
      : Buffer(std::move(Buffer)), OS(std::move(OS)) {}

  ~RawPCHContainerGenerator() override = default;

  void HandleTranslationUnit(ASTContext &Ctx) override {
    if (Buffer->IsComplete) {
      // Make sure it hits disk now.
      *OS << Buffer->Data;
      OS->flush();
    }
    // Free the space of the temporary buffer.
    llvm::SmallVector<char, 0> Empty;
    Buffer->Data = std::move(Empty);
  }
};

/// A PCHContainerGenerator that writes out the PCH to a flat file if the
/// action is needed (and the filename is determined at the time the output
/// is done).
class RawPCHDeferredContainerGenerator : public ASTConsumer {
  CompilerInstance &CI;
  std::shared_ptr<PCHBuffer> Buffer;

public:
  RawPCHDeferredContainerGenerator(CompilerInstance &CI, std::shared_ptr<PCHBuffer> Buffer)
      : CI(CI), Buffer(std::move(Buffer)) {}

  ~RawPCHDeferredContainerGenerator() override = default;

  void HandleTranslationUnit(ASTContext &Ctx) override {
    if (Buffer->IsComplete && !Buffer->PresumedFileName.empty()) {
//     llvm::dbgs () << "attempting to write" << Buffer->PresumedFileName << "\n";
      std::error_code Error;
      StringRef Parent = llvm::sys::path::parent_path(Buffer->PresumedFileName);
      Error = llvm::sys::fs::create_directory(Parent);
      if (!Error) {
        int FD;
        Error = llvm::sys::fs::openFileForWrite(Buffer->PresumedFileName, FD);
        if (!Error) {
          std::unique_ptr<raw_pwrite_stream> OS;
          OS.reset(new llvm::raw_fd_ostream(FD, /*shouldClose=*/true));
          // Make sure it hits disk now.
          *OS << Buffer->Data;
          OS->flush();
          if (ModuleClient *MC = CI.getMapper(SourceLocation())) {
            MC->Cork();
            MC->ModuleCompiled(Buffer->PresumedFileName);
            auto Response = MC->Uncork();
            if (Response[0].GetCode () == Cody::Client::PC_OK)
              ;
            else {
              assert(Response[0].GetCode () == Cody::Client::PC_ERROR &&
                      "not a path and not an error?");
            }
          }
        }
        else
        llvm::dbgs() << " Problem creating : " << Buffer->PresumedFileName << "\n";
      }
      else {
        llvm::dbgs() << " Problem creating dir : " << Parent << "\n";
      }
    } //else
    //llvm::dbgs () << "no module emitted\n";

    // Free the space of the temporary buffer.
    llvm::SmallVector<char, 0> Empty;
    Buffer->Data = std::move(Empty);
  }
};

} // anonymous namespace

std::unique_ptr<ASTConsumer> RawPCHContainerWriter::CreatePCHContainerGenerator(
    CompilerInstance &CI, const std::string &MainFileName,
    const std::string &OutputFileName, std::unique_ptr<llvm::raw_pwrite_stream> OS,
    std::shared_ptr<PCHBuffer> Buffer) const {
  return std::make_unique<RawPCHContainerGenerator>(std::move(OS), Buffer);
}

std::unique_ptr<ASTConsumer> RawPCHContainerWriter::CreatePCHDeferredContainerGenerator(
    CompilerInstance &CI, const std::string &MainFileName,
    const std::string &OutputFileName, std::unique_ptr<llvm::raw_pwrite_stream> OS,
    std::shared_ptr<PCHBuffer> Buffer) const {
  return std::make_unique<RawPCHDeferredContainerGenerator>(CI, Buffer);
}

StringRef
RawPCHContainerReader::ExtractPCH(llvm::MemoryBufferRef Buffer) const {
  return Buffer.getBuffer();
}

PCHContainerOperations::PCHContainerOperations() {
  registerWriter(std::make_unique<RawPCHContainerWriter>());
  registerReader(std::make_unique<RawPCHContainerReader>());
}
