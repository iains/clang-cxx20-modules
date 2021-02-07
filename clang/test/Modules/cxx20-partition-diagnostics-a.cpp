// Module Partition diagnostics

// RUN: %clang_cc1 -std=c++20 -S -D TU=1 -x c++ %s -o /dev/null -verify

// RUN: %clang_cc1 -std=c++20 -S -D TU=2 -x c++ %s -o /dev/null -verify

#if TU == 1

import :B; // expected-error {{module partition imports must be within a module purview}}

#elif TU == 2
module; // expected-error {{missing 'module' declaration at end of global module fragment introduced here}}

import :Part; // expected-error {{module partition imports cannot be in the global module fragment}}

#else
#error "no TU set"
#endif
