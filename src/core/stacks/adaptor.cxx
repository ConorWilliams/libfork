// export module libfork.core:adaptor_stack;
//
// import std;
//
// import :concepts;
//
// namespace lf::stacks {
//
// // TODO: use allocator
//
// export template <allocator_of<std::byte> Allocator = std::allocator<std::byte>>
// struct adaptor {
//
//   using allocator_type = std::allocator<std::byte>;
//
//   struct ckpt {
//     auto operator==(ckpt const &) const -> bool = default;
//   };
//
//   constexpr static auto push(std::size_t sz) -> void * { return new std::byte[sz]; }
//   constexpr static auto pop(void *p, std::size_t sz) noexcept -> void;
//   constexpr static auto checkpoint() noexcept -> ckpt;
//   constexpr static auto prepare_release() noexcept -> int;
//   constexpr static auto release(int) noexcept -> void;
//   constexpr static auto acquire(ckpt) noexcept -> void;
// };
//
// } // namespace lf::stacks
