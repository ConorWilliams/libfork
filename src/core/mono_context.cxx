export module libfork.core:mono_context;

import std;

import :concepts;
import :poly_context;

namespace lf {

template <typename Context>
class vector_stack {

 public:
  constexpr void push(steal_handle<Context> value) { m_vector.push_back(value); }

  constexpr auto pop() noexcept -> steal_handle<Context> {
    if (!m_vector.empty()) {
      steal_handle<Context> value = m_vector.back();
      m_vector.pop_back();
      return value;
    }
    return {};
  }

 private:
  std::vector<steal_handle<Context>> m_vector;
};

// TODO: allow customization of post (via Container?)

export template <                                         //
    worker_stack Stack,                                   //
    template <typename> typename Container = vector_stack //
    >
class mono_context : public base_context<Stack> {

  using sched_h = sched_handle<mono_context>;
  using steal_h = steal_handle<mono_context>;

  // TODO: Move some of these type defs into the base class

  using allocator_type = Stack::allocator_type;
  using allocator_traits = std::allocator_traits<allocator_type>;

  using container_type = Container<mono_context>;

 public:
  constexpr void push(steal_handle<mono_context> frame) { m_container.push(frame); }

  constexpr auto pop() noexcept -> steal_handle<mono_context> { return m_container.pop(); }

  // TODO: make allocator aware
  // TODO: make generic over vector/deque

 private:
  container_type m_container;
};

} // namespace lf
