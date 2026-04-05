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

// TODO: allow customization of post (via Container?)

export template <                                                   //
    worker_stack Stack,                                             //
    template <typename, typename> typename Container = vector_stack //
    >
class generic_context : public context_stack_base<Stack> {

  using sched_h = sched_handle<generic_context>;
  using steal_h = steal_handle<generic_context>;

  // TODO: Move some of these type defs into the base class

  using allocator_type = Stack::allocator_type;
  using allocator_traits = std::allocator_traits<allocator_type>;

  using container_type = Container<steal_h, typename allocator_traits::template rebind_alloc<steal_h>>;

 public:
  constexpr void push(steal_h frame) { m_container.push(frame); }

  constexpr auto pop() noexcept -> steal_h { return m_container.pop(); }

  // TODO: make allocator aware
  // TODO: make generic over vector/deque

 private:
  container_type m_container;
};

} // namespace lf
