export module libfork.core:fixed_stack;

import std;

import :utility;

namespace lf::stack {

struct fixed {

  std::unique_ptr<std::byte[]> data = std::make_unique<std::byte[]>(1024 * 1024);
  std::byte *ptr = data.get();

  constexpr auto push(std::size_t sz) -> void * {
    auto *prev = ptr;
    ptr += fib_align_size(sz);
    return prev;
  }
  constexpr auto pop(void *p, std::size_t) noexcept -> void { ptr = static_cast<std::byte *>(p); }

  constexpr auto checkpoint() noexcept -> std::byte * { return data.get(); }
  constexpr auto prepare_release() noexcept -> std::byte * { return data.get(); }
  constexpr auto release(std::byte *) noexcept -> void {}
  constexpr auto acquire(std::byte *) noexcept -> void {}
};

} // namespace lf::stack
