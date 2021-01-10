//===-- ModuleMapper.h - Interface to libcody module mapper -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_CLANG_FRONTEND_MODULEMAPPER_H
#define LLVM_CLANG_FRONTEND_MODULEMAPPER_H

#include "clang/Basic/SourceLocation.h"

// Mapper interface for client and server bits
#include "libcody/cody.hh"

// C++
#include <map>
#include <string>

namespace clang {

class ModuleResolver : public Cody::Resolver {
public:
  using Parent = Cody::Resolver;
  using ModuleMapping = std::map<std::string, std::string>;

private:
  std::string Repo;
  std::string Ident;
  ModuleMapping Map;
  int FdRepo = -1;
  bool DefaultMap = true;
  bool DefaultTranslate = true;

public:
  ModuleResolver(bool Map = true, bool Xlate = false);
  virtual ~ModuleResolver() override;

public:
  void setDefaultMap(bool D) { DefaultMap = D; }
  void setDefaultTranslate(bool D) { DefaultTranslate = D; }
  void setIdent(char const *I) { Ident = I; }
  bool setRepo(std::string &&Repo, bool Force = false);
  bool addMapping(std::string &&Module, std::string &&File, bool Force = false);

#if NOT_YET
  // Return +ve line number of error, or -ve errno
  int read_tuple_file(int fd, char const *prefix, bool force = false);
  int read_tuple_file(int fd, std::string const &prefix, bool force = false) {
    return read_tuple_file(fd, prefix.empty() ? nullptr : prefix.c_str(),
                           force);
  }
#endif

public:
  // Virtual overriders, names are controlled by Cody::Resolver
  virtual ModuleResolver *ConnectRequest(Cody::Server *, unsigned Version,
                                         std::string &Agent,
                                         std::string &Ident) override;
  virtual int ModuleRepoRequest(Cody::Server *S) override;
  virtual int ModuleExportRequest(Cody::Server *S, Cody::Flags,
                                  std::string &Module) override;
  virtual int ModuleImportRequest(Cody::Server *S, Cody::Flags,
                                  std::string &Module) override;
  virtual int IncludeTranslateRequest(Cody::Server *S, Cody::Flags,
                                      std::string &Include) override;

private:
  virtual char const *GetCMISuffix() override;

private:
  int cmiResponse(Cody::Server *S, std::string &Module);
};

class ModuleClient : public Cody::Client {
  std::string ModuleRepositoryName = "";

public:
  ModuleClient(Cody::Server *S) : Client(S) {}
#if NOT_YET
  ModuleClient(int FdFrom, int FdTo) : Client(FdFrom, FdTo) {}
#endif

public:
  void setModuleRepositoryName (const char *R);
  std::string maybeAddRepoPrefix(std::string ModPathIn);
  static ModuleClient *openModuleClient(SourceLocation Loc,
                                        std::string Option,
                                        char const *ProgName);
  static void closeModuleClient(SourceLocation Loc, ModuleClient *);
};

} // namespace clang
#endif // LLVM_CLANG_FRONTEND_MODULEMAPPER_H