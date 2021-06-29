// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: %clang_cc1 -std=c++2a -I%S -emit-module-interface -DINTERFACE %s -o %t.pcm
// RUN: %clang_cc1 -std=c++2a -I%S -fmodule-file=%t.pcm -DIMPLEMENTATION %s -verify -fno-modules-error-recovery
// RUN: %clang_cc1 -std=c++2a -I%S -fmodule-file=%t.pcm %s -verify -fno-modules-error-recovery

#ifdef INTERFACE
module;
// extern int in_header; OK
#include "p2-h1.h"
// int global_module_fragment; A violation of ODR
#include "p2-h2.h"

//inline int fortytwo () { return 42; }

export module A;

export int exported;
int not_exported;
static int internal;

#if __clang__
module :private;
int not_exported_private;
static int internal_private;
#endif

#else

#ifdef IMPLEMENTATION
module;
#endif

// a function in the GMF attempting to access various vars.
#include "p2-h3.h"
//extern int kap;
//inline int fortytwo () { return 42; }

#ifdef IMPLEMENTATION
module A;
#else
import A;
#endif

void test_late() {
  in_header = 1; // expected-error {{use of undeclared identifier 'in_header'}}

  global_module_fragment = 1; // expected-error {{use of undeclared identifier 'global_module_fragment'}}

  exported = 1;

  not_exported = 1;
#ifndef IMPLEMENTATION
  // expected-error@-2 {{undeclared identifier 'not_exported'; did you mean 'exported'}}
  // expected-note@p2.cpp:18 {{declared here}}
#endif

  internal = 1;
#ifndef IMPLEMENTATION
  // Not visible to non-implementing importer
  // expected-error@-3 {{undeclared identifier}}
#endif

#if __clang__
  not_exported_private = 1;
# ifndef IMPLEMENTATION
  // Not visible to non-implementing importer
  // expected-error@-3 {{undeclared identifier}}
# endif

  internal_private = 1;
# ifndef IMPLEMENTATION
  // Not visible to non-implementing importer
  // expected-error@-3 {{undeclared identifier}}
# endif
#endif
}

#endif
