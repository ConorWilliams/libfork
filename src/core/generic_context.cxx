export module libfork.core:generic_context;

import std;

import :concepts;
import :poly_context;

namespace lf {

template <typename T, typename Allocator>
class vector_stack {

 public:
  constexpr void push(T value) { m_vector.push_back(value); }

  constexpr auto pop() noexcept -> T {
    if (!m_vector.empty()) {
      T value = m_vector.back();
      m_vector.pop_back();
      return value;
    }
    return {};
  }

 private:
  std::vector<T, Allocator> m_vector;
};

export template <                                                  //
    bool Polymorphic,                                              //
    worker_stack Stack,                                            //
    template <typename, typename> typename Container = std::vector //
    >
class inline_context final : context_base<Polymorphic, Stack> {

  using base_type = context_base<Polymorphic, Stack>;

 public:
  using context_type = std::conditional_t<Polymorphic, base_type, inline_context>;

 private:
  using await_h = sched_handle<context_type>;
  using frame_h = steal_handle<context_type>;

  using allocator_type = Stack::allocator_type;
  using allocator_traits = std::allocator_traits<allocator_type>;
  using allocator_handle = allocator_traits::template rebind_alloc<frame_h>;

 public:
  /**
   * @brief Get a view of this object as a context.
   */
  constexpr auto context() noexcept -> base_type & { return this; }

  using base_type::stack;

  constexpr void push(frame_h frame) { m_container.push_back(frame); }

  constexpr auto pop() noexcept -> frame_h {
    if (!m_container.empty()) {
      frame_h frame = m_container.back();
      m_container.pop_back();
      return frame;
    }
    return {};
  }

  constexpr void post(await_h frame) noexcept(!Polymorphic) {
    LF_ASSERT(frame);
    //
  }

  // TODO: make allocator aware
  // TODO: make generic over vector/deque

  // explicit constexpr inline_context(allocator_type const &) noexcept;

 private:
  Container<steal_handle<inline_context>, allocator_handle> m_container;
};

} // namespace lf
