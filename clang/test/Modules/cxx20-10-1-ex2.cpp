
// RUN: rm -rf %t
// RUN: mkdir -p %t

// RUN: %clang_cc1 -std=c++20 -emit-module-interface -D TU=1 -x c++ %s \
// RUN:  -o %t/B_Y.pcm

// RUN: %clang_cc1 -std=c++20 -emit-module-interface -D TU=2 -x c++ %s \
// RUN:  -fmodule-file=%t/B_Y.pcm -o %t/B.pcm

// RUN: %clang_cc1 -std=c++20 -emit-module-interface -D TU=3 -x c++ %s \
// RUN:   -o %t/B_X1.pcm -verify

// Not expected to work yet.
//  %clang_cc1 -std=c++20 -emit-module-interface -D TU=4 -x c++ %s \
//   -fmodule-file=%t/B.pcm  -o %t/B_X2.pcm

// RUN: %clang_cc1 -std=c++20 -emit-obj -D TU=5 -x c++ %s \
// RUN:  -fmodule-file=%t/B.pcm  -o %t/b_tu5.s

// RUN: %clang_cc1 -std=c++20 -S -D TU=6 -x c++ %s \
// RUN:  -fmodule-file=%t/B.pcm  -o %t/b_tu6.s -verify

// Not expected to work yet.
//  %clang_cc1 -std=c++20 -emit-module-interface -D TU=7 -x c++ %s \
//   -fmodule-file=%t/B_X2.pcm  -o %t/B_X3.pcm -verify

#if TU == 1
  module B:Y;
  int y();
// expected-no-diagnostics
#elif TU == 2
  export module B;
  import :Y;
  int n = y();
// expected-no-diagnostics
#elif TU == 3
  module B:X1; // does not implicitly import B
  int &a = n;  // expected-error {{use of undeclared identifier }}
#elif TU == 4
  module B:X2; // does not implicitly import B
  import B;
  int &b = n; // OK
// expected-no-diagnostics
#elif TU == 5
  module B; // implicitly imports B
  int &c = n; // OK
// expected-no-diagnostics
#elif TU == 6
  import B;
              // error, n is module-local and this is not a module.
  int &c = n; // expected-error {{use of undeclared identifier}}
#elif TU == 7
  module B:X3; // does not implicitly import B
  import :X2; // X2 is an implementation so exports nothing.
              // error: n not visible here.
  int &c = n; // expected-error {{use of undeclared identifier }}
#else
#error "no TU set"
#endif
