#pragma once

// This file has been adapted from the following source: https://godbolt.org/

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "libfork/utility.hpp"

// NOLINTBEGIN

namespace lf::detail {

struct alignas(__STDCPP_DEFAULT_NEW_ALIGNMENT__) aligned_block {
  unsigned char m_pad[__STDCPP_DEFAULT_NEW_ALIGNMENT__];
};

template <class Alloc>
using rebind = typename std::allocator_traits<Alloc>::template rebind_alloc<aligned_block>;

template <class Alloc>
concept has_real_pointers = std::same_as<Alloc, void> || std::is_pointer_v<typename std::allocator_traits<Alloc>::pointer>;

template <class Alloc = void>
class allocator_mixin {
 private:
  using AlignedAlloc = rebind<Alloc>;

  static void* allocate(AlignedAlloc alloc, std::size_t const size) {
    if constexpr (std::default_initializable<AlignedAlloc> && std::allocator_traits<AlignedAlloc>::is_always_equal::value) {
      // Do not store stateless allocator.
      std::size_t const count = (size + sizeof(aligned_block) - 1) / sizeof(aligned_block);
      return alloc.allocate(count);
    } else {
      // Store stateful allocator.
      static constexpr std::size_t align = std::max(alignof(AlignedAlloc), sizeof(aligned_block));
      std::size_t const count = (size + sizeof(AlignedAlloc) + align - 1) / sizeof(aligned_block);
      void* const ptr = alloc.allocate(count);
      auto const al_address = (reinterpret_cast<std::uintptr_t>(ptr) + size + alignof(AlignedAlloc) - 1) & ~(alignof(AlignedAlloc) - 1);
      ::new (reinterpret_cast<void*>(al_address)) AlignedAlloc(std::move(alloc));
      return ptr;
    }
  }

 public:
  static void* operator new(const std::size_t size)
  requires std::default_initializable<AlignedAlloc>
  {
    return allocate(AlignedAlloc{}, size);
  }

  template <class A2, class... Args>
  requires std::convertible_to<const A2&, Alloc>
  static void* operator new(const std::size_t size, std::allocator_arg_t, const A2& alloc, Args const&...) {
    return allocate(static_cast<AlignedAlloc>(static_cast<Alloc>(alloc)), size);
  }

  template <class This, class A2, class... Args>
  requires std::convertible_to<const A2&, Alloc>
  static void* operator new(const std::size_t size, This const&, std::allocator_arg_t, const A2& alloc, Args const&...) {
    return allocate(static_cast<AlignedAlloc>(static_cast<Alloc>(alloc)), size);
  }

  static void operator delete(void* const ptr, const std::size_t size) noexcept {
    if constexpr (std::default_initializable<AlignedAlloc> && std::allocator_traits<AlignedAlloc>::is_always_equal::value) {
      // Make stateless allocator.
      AlignedAlloc alloc{};
      const std::size_t count = (size + sizeof(aligned_block) - 1) / sizeof(aligned_block);
      alloc.deallocate(static_cast<aligned_block*>(ptr), count);
    } else {
      // Retrieve stateful allocator.
      auto const al_address = (reinterpret_cast<std::uintptr_t>(ptr) + size + alignof(AlignedAlloc) - 1) & ~(alignof(AlignedAlloc) - 1);
      auto& stored_al = *reinterpret_cast<AlignedAlloc*>(al_address);
      AlignedAlloc alloc{std::move(stored_al)};
      stored_al.~AlignedAlloc();

      static constexpr std::size_t align = std::max(alignof(AlignedAlloc), sizeof(aligned_block));
      const std::size_t count = (size + sizeof(AlignedAlloc) + align - 1) / sizeof(aligned_block);
      alloc.deallocate(static_cast<aligned_block*>(ptr), count);
    }
  }
};

template <>
class allocator_mixin<void> {  // type-erased allocator
 private:
  using dealloc_fn = void (*)(void*, std::size_t);

  template <class AnyAlloc>
  static void* allocate(AnyAlloc const& proto, std::size_t size) {
    using AlignedAlloc = rebind<AnyAlloc>;
    auto alloc = static_cast<AlignedAlloc>(proto);

    if constexpr (std::default_initializable<AlignedAlloc> && std::allocator_traits<AlignedAlloc>::is_always_equal::value) {
      // Don't store stateless allocator.
      const dealloc_fn dealloc = [](void* const ptr, const std::size_t size) {
        AlignedAlloc alloc{};
        const std::size_t count = (size + sizeof(dealloc_fn) + sizeof(aligned_block) - 1) / sizeof(aligned_block);
        alloc.deallocate(static_cast<aligned_block*>(ptr), count);
      };

      const std::size_t count = (size + sizeof(dealloc_fn) + sizeof(aligned_block) - 1) / sizeof(aligned_block);
      void* const ptr = alloc.allocate(count);
      ::memcpy(static_cast<char*>(ptr) + size, &dealloc, sizeof(dealloc));
      return ptr;
    } else {
      // Store stateful allocator.
      static constexpr std::size_t align = std::max(alignof(AlignedAlloc), sizeof(aligned_block));

      const dealloc_fn dealloc = [](void* const ptr, std::size_t size) {
        size += sizeof(dealloc_fn);
        const auto al_address = (reinterpret_cast<std::uintptr_t>(ptr) + size + alignof(AlignedAlloc) - 1) & ~(alignof(AlignedAlloc) - 1);
        auto& stored_al = *reinterpret_cast<const AlignedAlloc*>(al_address);
        AlignedAlloc alloc{std::move(stored_al)};
        stored_al.~AlignedAlloc();

        const std::size_t count = (size + sizeof(alloc) + align - 1) / sizeof(aligned_block);
        alloc.deallocate(static_cast<aligned_block*>(ptr), count);
      };

      const std::size_t count = (size + sizeof(dealloc_fn) + sizeof(alloc) + align - 1) / sizeof(aligned_block);
      void* const ptr = alloc.allocate(count);
      ::memcpy(static_cast<char*>(ptr) + size, &dealloc, sizeof(dealloc));
      size += sizeof(dealloc_fn);
      auto const al_address = (reinterpret_cast<std::uintptr_t>(ptr) + size + alignof(AlignedAlloc) - 1) & ~(alignof(AlignedAlloc) - 1);
      ::new (reinterpret_cast<void*>(al_address)) AlignedAlloc{std::move(alloc)};
      return ptr;
    }
  }
  //
 public:
  static void* operator new(const std::size_t size) {  // default: new/delete
    void* const ptr = ::operator new[](size + sizeof(dealloc_fn));
    const dealloc_fn dealloc = [](void* const ptr, const std::size_t size) {
#if __cpp_sized_deallocation
      ::operator delete[](ptr, size + sizeof(dealloc_fn));
#else
      ASSERT_ASSUME(false, "sized deallocation not supported");
#endif
    };
    ::memcpy(static_cast<char*>(ptr) + size, &dealloc, sizeof(dealloc_fn));
    return ptr;
  }

  template <class AnyAlloc, class... Args>
  static void* operator new(const std::size_t size, std::allocator_arg_t, AnyAlloc const& alloc, Args const&...) {
    static_assert(has_real_pointers<AnyAlloc>, "coroutine allocators must use true pointers");
    return allocate(alloc, size);
  }

  template <class This, class AnyAlloc, class... Args>
  static void* operator new(const std::size_t size, This const&, std::allocator_arg_t, AnyAlloc const& alloc, Args const&...) {
    static_assert(has_real_pointers<AnyAlloc>, "coroutine allocators must use true pointers");
    return allocate(alloc, size);
  }

  static void operator delete(void* const ptr, const std::size_t size) noexcept {
    dealloc_fn dealloc;
    ::memcpy(&dealloc, static_cast<char const*>(ptr) + size, sizeof(dealloc_fn));
    dealloc(ptr, size);
  }
};

}  // namespace lf::detail

// NOLINTEND