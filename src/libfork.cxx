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
