//===--- ModuleMapperResolver.cpp - Resolver interface to libcody ------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  A resolver interface for Modules using the libCODY implementation.
//
//===----------------------------------------------------------------------===//

#include "clang/Basic/SourceLocation.h"

#include "clang/Frontend/ModuleMapper.h"

using namespace clang;

// C++
#include <algorithm>
// C
#include <cstring>
// OS
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

ModuleResolver::ModuleResolver(bool Map, bool Xlate)
    : DefaultMap(Map), DefaultTranslate(Xlate) {}

ModuleResolver::~ModuleResolver() {
  if (FdRepo >= 0)
    close (FdRepo);
}

bool ModuleResolver::setRepo(std::string &&R, bool Force) {
  if (Force || Repo.empty()) {
    Repo = std::move(R);
    Force = true;
  }
  return Force;
}

bool ModuleResolver::addMapping(std::string &&Module, std::string &&File,
                                bool Force) {
  auto Res = Map.emplace(std::move(Module), std::move(File));
  if (Res.second)
    Force = true;
  else if (Force)
    Res.first->second = std::move(File);

  return Force;
}

#if NOT_YET
int ModuleResolver::read_tuple_file(int fd, char const *prefix, bool force) {
  struct stat stat;
  if (fstat(fd, &stat) < 0)
    return -errno;

  if (!stat.st_size)
    return 0;

  // Just map the file, we're gonna read all of it, so no need for
  // line buffering
  void *buffer = mmap(nullptr, stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (buffer == MAP_FAILED)
    return -errno;

  size_t prefix_len = prefix ? strlen(prefix) : 0;
  unsigned lineno = 0;

  for (char const *begin = reinterpret_cast<char const *>(buffer),
                  *end = begin + stat.st_size, *eol;
       begin != end; begin = eol + 1) {
    lineno++;
    eol = std::find(begin, end, '\n');
    if (eol == end)
      // last line has no \n, ignore the line, you lose
      break;

    auto *pos = begin;
    bool pfx_search = prefix_len != 0;

  pfx_search:
    while (*pos == ' ' || *pos == '\t')
      pos++;

    auto *space = pos;
    while (*space != '\n' && *space != ' ' && *space != '\t')
      space++;

    if (pos == space)
      // at end of line, nothing here
      continue;

    if (pfx_search) {
      if (size_t(space - pos) == prefix_len && std::equal(pos, space, prefix))
        pfx_search = false;
      pos = space;
      goto pfx_search;
    }

    std::string Module(pos, space);
    while (*space == ' ' || *space == '\t')
      space++;
    std::string file(space, eol);

    if (Module[0] == '$') {
      if (Module == "$root")
        set_repo(std::move(file));
      else
        return lineno;
    } else {
      if (file.empty())
        file = GetCMIName(Module);
      addMapping(std::move(Module), std::move(file), force);
    }
  }

  munmap(buffer, stat.st_size);

  return 0;
}
#endif

char const *ModuleResolver::GetCMISuffix() { return "pcm"; }

ModuleResolver *ModuleResolver::ConnectRequest(Cody::Server *S,
                                               unsigned Version, std::string &A,
                                               std::string &I) {
  if (!Version || Version > Cody::Version)
    S->ErrorResponse("version mismatch");
  else if (A != "clang")
    // Refuse anything but clang for this implementation
    ErrorResponse(S, std::string("only clang supported"));
  else if (!Ident.empty() && Ident != I)
    // Failed ident check
    ErrorResponse(S, std::string("bad ident"));
  else
    // Success!
    S->ConnectResponse("clang");

  return this;
}

int ModuleResolver::ModuleRepoRequest(Cody::Server *S) {
  S->PathnameResponse(Repo);
  return 0;
}

int ModuleResolver::cmiResponse(Cody::Server *S, std::string &Module) {
  auto Iter = Map.find(Module);
  if (Iter == Map.end()) {
    std::string File;
    if (DefaultMap)
      File = GetCMIName(Module);
    auto res = Map.emplace(Module, File);
    Iter = res.first;
  }

  if (Iter->second.empty())
    S->ErrorResponse("no such Module");
  else
    S->PathnameResponse(Iter->second);

  return 0;
}

int ModuleResolver::ModuleExportRequest(Cody::Server *S, Cody::Flags,
                                        std::string &Module) {
  return cmiResponse(S, Module);
}

int ModuleResolver::ModuleImportRequest(Cody::Server *S, Cody::Flags,
                                        std::string &Module) {
  return cmiResponse(S, Module);
}

int ModuleResolver::IncludeTranslateRequest(Cody::Server *S, Cody::Flags,
                                            std::string &Include) {
  auto Iter = Map.find(Include);
  if (Iter == Map.end() && DefaultTranslate) {
    // Not found, look for it
    if (FdRepo == -1 && !Repo.empty()) {
      FdRepo = open(Repo.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY);
      if (FdRepo < 0)
        FdRepo = -2;
    }

    if (FdRepo >= 0 || Repo.empty()) {
      auto File = GetCMIName(Include);
      struct stat Statbuf;
      if (fstatat(Repo.empty() ? AT_FDCWD : FdRepo, File.c_str(), &Statbuf, 0) <
              0 ||
          !S_ISREG(Statbuf.st_mode))
        // Mark as not present
        File.clear();
      auto Res = Map.emplace(Include, File);
      Iter = Res.first;
    }
  }

  if (Iter == Map.end() || Iter->second.empty())
    S->BoolResponse(false);
  else
    S->PathnameResponse(Iter->second);

  return 0;
}
