export module libfork.batteries:mono_context;

import std;

import libfork.core;
import libfork.core;

// TODO: search for places that HOF would simplify

namespace lf {

// TODO: allow customization of post (via Container?)
// TODO: allocator aware

export template <                        //
    worker_stack Stack,                  //
    template <typename> typename Adaptor //
    >
class mono_context : public base_context<Stack> {
 public:
  using context_type = mono_context;

  constexpr void push(steal_handle<context_type> frame) noexcept(noexcept(m_container.push(frame))) {
    m_container.push(frame);
  }

  constexpr auto pop() noexcept -> steal_handle<context_type> { return m_container.pop(); }

  constexpr void post(sched_handle<context_type> handle) noexcept(noexcept(m_container.post(handle)))
    requires requires (Adaptor<context_type> context) {
      { context.post(handle) } -> std::same_as<void>;
    }
  {
    m_container.post(handle);
  }

 private:
  Adaptor<context_type> m_container;
};

} // namespace lf
