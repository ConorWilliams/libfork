module;
export module libfork.core:dummy_stack;

import std;

namespace lf {

export struct dummy_allocator {

  struct ckpt {
    auto operator==(ckpt const &) const -> bool = default;
  };

  constexpr static auto push(std::size_t sz) -> void *;
  constexpr static auto pop(void *p, std::size_t sz) noexcept -> void;
  constexpr static auto checkpoint() noexcept -> ckpt;
  constexpr static auto prepare_release() noexcept -> int;
  constexpr static auto release(int) noexcept -> void;
  constexpr static auto acquire(ckpt) noexcept -> void;
};

} // namespace lf
