// RUN: rm -rf %t
// RUN: mkdir -p %t

// RUN: %clang_cc1 -std=c++20 -emit-module-interface -D TU=1 -x c++ %s \
// RUN:  -o %t/M_PartImpl.pcm

// RUN: %clang_cc1 -std=c++20 -emit-module-interface -D TU=2 -x c++ %s \
// RUN:  -fmodule-file=%t/M_PartImpl.pcm -o %t/M.pcm -verify

// RUN: %clang_cc1 -std=c++20 -emit-module-interface -D TU=3 -x c++ %s \
// RUN:  -o %t/M_Part.pcm

// RUN: %clang_cc1 -std=c++20 -emit-module-interface -D TU=4 -x c++ %s \
// RUN:  -fmodule-file=%t/M_Part.pcm -o %t/M.pcm

#if TU == 1
module M:PartImpl;

// expected-no-diagnostics
#elif TU == 2
export module M;
                     // error: exported partition :Part is an implementation unit
export import :PartImpl; // expected-error {{module partition implementations cannot be exported}}

#elif TU == 3
export module M:Part;

// expected-no-diagnostics
#elif TU == 4
export module M;
export import :Part;

// expected-no-diagnostics
#else
#error "no TU set"
#endif
