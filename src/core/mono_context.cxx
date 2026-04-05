export module libfork.core:mono_context;

import std;

import :concepts;
import :poly_context;

// TODO: search for places that HOF would simplify

namespace lf {

template <typename Context>
class vector_adaptor {
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
// TODO: allocator aware

export template <                                           //
    worker_stack Stack,                                     //
    template <typename> typename Container = vector_adaptor //
    >
class mono_context : public base_context<Stack> {
 public:
  using context_type = mono_context;

  constexpr void push(steal_handle<context_type> frame) noexcept(noexcept(m_container.push(frame))) {
    m_container.push(frame);
  }

  constexpr auto pop() noexcept -> steal_handle<context_type> { return m_container.pop(); }

  constexpr void post(sched_handle<context_type> handle) noexcept(noexcept(m_container.post(handle)))
    requires requires (Container<context_type> context) {
      { context.post(handle) } -> std::same_as<void>;
    }
  {
    m_container.post(handle);
  }

 private:
  Container<context_type> m_container;
};

} // namespace lf
