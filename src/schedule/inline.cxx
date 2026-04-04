module;
export module libfork.schedule:inline_context;

import std;

import libfork.core;

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
class inline_context final : public context_base<Polymorphic, Stack> {

  using context_type = std::conditional_t<Polymorphic, context_base<Polymorphic, Stack>, inline_context>;

  using await_h = sched_handle<context_type>;
  using frame_h = steal_handle<context_type>;

  using allocator_type = Stack::allocator_type;
  using allocator_traits = std::allocator_traits<allocator_type>;
  using allocator_handle = allocator_traits::template rebind_alloc<frame_h>;

 public:
  constexpr auto poly() noexcept -> context_base<Polymorphic, Stack> &
    requires Polymorphic
  {
    return this;
  }

  constexpr void post(await_h frame) noexcept(!Polymorphic) {
    LF_ASSERT(frame);

    //
  }

  constexpr void push(frame_h frame) { m_stack.push_back(frame); }

  constexpr auto pop() noexcept -> frame_h {
    if (!m_stack.empty()) {
      frame_h frame = m_stack.back();
      m_stack.pop_back();
      return frame;
    }
    return {};
  }

  // TODO: make allocator aware
  // TODO: make generic over vector/deque

  // explicit constexpr inline_context(allocator_type const &) noexcept;

 private:
  Container<steal_handle<inline_context>, allocator_handle> m_stack;
};

} // namespace lf
