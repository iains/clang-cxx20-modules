#ifndef P2_H3_H
#define P2_H3_H

inline void test_early() {
  in_header = 1; // expected-error {{use of undeclared identifier 'in_header'}}

  global_module_fragment = 1; // expected-error {{use of undeclared identifier 'global_module_fragment'}}

  exported = 1; // expected-error {{must be imported from module 'A'}}
  // expected-note@p2.cpp:18 {{declaration here is not visible}}

  not_exported = 1; // expected-error {{undeclared identifier}}

  internal = 1; // expected-error {{undeclared identifier}}

#if __clang__
  not_exported_private = 1; // expected-error {{undeclared identifier}}

  internal_private = 1; // expected-error {{undeclared identifier}}
#endif
}

#endif
