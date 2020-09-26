//===--- ModuleMapperClient.cpp - Client interface to libcody ------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  A client interface for modules using the libCODY implementation.
//
//===----------------------------------------------------------------------===//

#include "clang/Basic/SourceLocation.h"

#include "clang/Frontend/ModuleMapper.h"
#include "llvm/Support/Path.h"

using namespace clang;

#if NOT_YET
ModuleClient::ModuleClient(pex_obj *p, int fd_from, int fd_to)
    : Client(fd_from, fd_to), pex(p) {}

static ModuleClient *spawn_mapper_program(char const **errmsg,
                                          std::string &Name,
                                          char const *full_program_Name) {
  /* Split writable at white-space.  No space-containing args for
     you!  */
  // At most every other char could be an argument
  char **argv = new char *[Name.size() / 2 + 2];
  unsigned arg_no = 0;
  char *str = new char[Name.size()];
  memcpy(str, Name.c_str() + 1, Name.size());

  for (auto ptr = str;; ++ptr) {
    while (*ptr == ' ')
      ptr++;
    if (!*ptr)
      break;
    argv[arg_no++] = ptr;
    while (*ptr && *ptr != ' ')
      ptr++;
    if (!*ptr)
      break;
    *ptr = 0;
  }
  argv[arg_no] = nullptr;

  auto *pex = pex_init(PEX_USE_PIPES, progName, NULL);
  FILE *to = pex_input_pipe(pex, false);
  Name = argv[0];
  if (!to)
    *errmsg = "connecting input";
  else {
    int flags = PEX_SEARCH;

    if (full_program_Name) {
      /* Prepend the invoking path.  */
      size_t dir_len = progName - full_program_Name;
      std::string argv0;
      argv0.reserve(dir_len + Name.size());
      argv0.append(full_program_Name, dir_len).append(Name);
      Name = std::move(argv0);
      argv[0] = const_cast<char *>(Name.c_str());
      flags = 0;
    }
    int err;
    *errmsg = pex_run(pex, flags, argv[0], argv, NULL, NULL, &err);
  }
  delete[] str;
  delete[] argv;

  int fd_from = -1, fd_to = -1;
  if (!*errmsg) {
    FILE *from = pex_read_output(pex, false);
    if (from && (fd_to = dup(fileno(to))) >= 0)
      fd_from = fileno(from);
    else
      *errmsg = "connecting output";
    fclose(to);
  }

  if (*errmsg) {
    pex_free(pex);
    return nullptr;
  }

  return new ModuleClient(pex, fd_from, fd_to);
}
#endif

ModuleClient *ModuleClient::openModuleClient(SourceLocation /*Loc*/,
                                             const char * /*O*/,
                                             char const * /*FullProgramName*/) {
  ModuleClient *C = nullptr;
  std::string Ident;
#if NOT_YET
  std::string Name;
  char const *errmsg = nullptr;
  unsigned Line = 0;

  if (O && O[0]) {
    /* Maybe a local or ipv6 address.  */
    Name = O;
    auto last = Name.find_last_of('?');
    if (last != Name.npos) {
      ident = Name.substr(last + 1);
      Name.erase(last);
    }

    if (Name.size()) {
      switch (Name[0]) {
      case '<':
        // <from>to or <>fromto, or <>
        {
          int fd_from = -1, fd_to = -1;
          char const *ptr = Name.c_str();
          char *eptr;

          fd_from = strtoul(++ptr, &eptr, 0);
          if (*eptr == '>') {
            ptr = eptr;
            fd_to = strtoul(++ptr, &eptr, 0);
            if (eptr != ptr && ptr == Name.c_str() + 1)
              fd_from = fd_to;
          }

          if (*eptr)
            errmsg = "parsing";
          else {
            if (Name.size() == 2) {
              fd_from = fileno(stdin);
              fd_to = fileno(stdout);
            }
            c = new ModuleClient(fd_from, fd_to);
          }
        }
        break;

      case '=':
        // =localsocket
        {
          int fd = Cody::OpenLocal(&errmsg, Name.c_str() + 1);
          if (fd >= 0)
            c = new ModuleClient(fd, fd);
        }
        break;

      case '|':
        // |program and args
        c = spawn_mapper_program(&errmsg, Name, full_program_Name);
        break;

      default:
        // file or hostName:port
        {
          auto colon = Name.find_last_of(':');
          if (colon != Name.npos) {
            char const *cptr = Name.c_str() + colon;
            char *endp;
            unsigned port = strtoul(cptr + 1, &endp, 10);

            if (port && endp != cptr + 1 && !*endp) {
              Name[colon] = 0;
              int fd = Cody::OpenInet6(&errmsg, Name.c_str(), port);
              Name[colon] = ':';

              if (fd >= 0)
                c = new ModuleClient(fd, fd);
            }
          }
        }
        break;
      }
    }
  }
#endif
  if (!C) {
    // Make a default in-process client
    bool IsFile = false; //! errmsg && !Name.empty ();
    auto R = new ModuleResolver(!IsFile, true);
#if NOT_YET

    if (file) {
      int fd = open(Name.c_str(), O_RDONLY | O_CLOEXEC);
      if (fd < 0)
        errmsg = "opening";
      else {
        if (int l = r->read_tuple_file(fd, ident, false)) {
          if (l > 0)
            line = l;
          errmsg = "reading";
        }

        close(fd);
      }
    } else
#endif
      R->setRepo("pcm-cache");

    auto *S = new Cody::Server(R);
    C = new ModuleClient(S);
  }

#if NOT_YET
//  if (errmsg)
//    error_at (loc, line ? G_("failed %s mapper %qs line %u")
//	      : G_("failed %s mapper %qs"), errmsg, Name.c_str (), line);
#endif

  // now wave hello!
  C->Cork();
  C->Connect(std::string("clang"), Ident);
  C->ModuleRepo();
  auto Packets = C->Uncork();

  auto &Connect = Packets[0];
  if (Connect.GetCode() == Cody::Client::PC_ERROR)
    assert(0 && "need an error message for failed handshake");
  //    error_at (loc, "failed mapper handshake %s", connect.GetString ().c_str
  //    ());
  assert (Connect.GetCode() == Cody::Client::PC_CONNECT &&
	  "missed a check for some other client code");

  auto &Repo = Packets[1];
  if (Repo.GetCode() == Cody::Client::PC_PATHNAME)
    C->setModuleRepositoryName(Repo.GetString().c_str());

  return C;
}

void ModuleClient::closeModuleClient(SourceLocation /*Loc*/,
                                     ModuleClient *Mapper) {
  if (Mapper->IsDirect()) {
    auto *S = Mapper->GetServer();
    auto *R = S->GetResolver();
    delete S;
    delete R;
  }
#if NOT_YET
  else {
    if (mapper->pex) {
      int fd_write = mapper->GetFDWrite();
      if (fd_write >= 0)
        close(fd_write);

      int status;
      pex_get_status(mapper->pex, 1, &status);

      pex_free(mapper->pex);
      mapper->pex = NULL;

      if (WIFSIGNALED(status))
        error_at(loc, "mapper died by signal %s", strsignal(WTERMSIG(status)));
      else if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        error_at(loc, "mapper exit status %d", WEXITSTATUS(status));
    } else {
      int fd_read = mapper->GetFDRead();
      close(fd_read);
    }
  }
#endif

  delete Mapper;
}

void ModuleClient::setModuleRepositoryName (const char *R) {
  // Match the behaviour from GCC for a name of '.'.
  if (!R || !R[0] || (R[0] == '.' && strlen(R) == 1))
    ModuleRepositoryName = "";
  else {
    ModuleRepositoryName = R;
    StringRef Sep = llvm::sys::path::get_separator();
    assert(Sep.size() == 1 && "not expecting to deal with multi-char sep");
    if (ModuleRepositoryName.back() == Sep[0])
      ModuleRepositoryName.pop_back();
  }
}

std::string ModuleClient::maybeAddRepoPrefix(std::string ModPathIn)
{
  if (!ModuleRepositoryName.empty() &&
      !llvm::sys::path::is_absolute(ModPathIn)) {
      std::string Out = ModuleRepositoryName;
    Out += llvm::sys::path::get_separator();
    Out += llvm::sys::path::remove_leading_dotslash(ModPathIn);
    return Out;
  } else
    return ModPathIn;
}

