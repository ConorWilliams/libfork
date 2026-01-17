// Global module fragment where #includes can happen
module;
export module libfork;

import std;

export class foo {
 public:
  foo();
  ~foo();
  void helloworld();
};

foo::foo() = default;
foo::~foo() = default;
void foo::helloworld() { std::cout << "hello world\n"; }
