// RUN: rm -rf %t
// RUN: mkdir -p %t

// RUN: %clang_cc1 -std=c++20 -emit-module-interface -D TU=0 -x c++ %s \
// RUN:  -o %t/M.pcm

// RUN: %clang_cc1 -std=c++20 -S -D TU=1 -x c++ %s \
// RUN:  -fmodule-file=%t/M.pcm -o %t/tu_8.s -verify

// RUN: %clang_cc1 -std=c++20 -emit-module-interface -D TU=2 -x c++ %s \
// RUN:  -o %t/M.pcm -verify

#if TU == 0
export module M;

#elif TU == 1
module M;
          // error: cannot import M in its own unit
import M; // expected-error {{import of module 'M' appears within its own implementation}}

#elif TU == 2
export module M;
          // error: cannot import M in its own unit
import M; // expected-error {{import of module 'M' appears within its own interface}}

#else
#error "no TU set"
#endif
