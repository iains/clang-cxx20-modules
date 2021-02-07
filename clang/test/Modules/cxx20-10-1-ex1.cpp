// The example in the standard is not in required build order.
// revised here

// RUN: rm -rf %t
// RUN: mkdir -p %t

// RUN: %clang_cc1 -std=c++20 -emit-module-interface -D TU=1 -x c++ %s \
// RUN:  -o %t/A_Internals.pcm

// RUN: %clang_cc1 -std=c++20 -emit-module-interface -D TU=2 -x c++ %s \
// RUN:  -fmodule-file=%t/A_Internals.pcm -o %t/A_Foo.pcm

// RUN: %clang_cc1 -std=c++20 -emit-module-interface -D TU=3 -x c++ %s \
// RUN:  -fmodule-file=%t/A_Foo.pcm -o %t/A.pcm

// RUN: %clang_cc1 -std=c++20 -emit-obj  -D TU=4 -x c++ %s \
// RUN:  -fmodule-file=%t/A.pcm -o %t/ex1.o

// expected-no-diagnostics

#if TU == 1

module A:Internals;
int bar();

#elif TU == 2

export module A:Foo;

import :Internals;

export int foo() { return 2 * (bar() + 1); }

#elif TU == 3

export module A;
export import :Foo;
export int baz();

#elif TU == 4
module A;

import :Internals;

int bar() { return baz() - 10; }
int baz() { return 30; }

#else
#error "no TU set"
#endif
