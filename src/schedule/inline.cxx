export module libfork.schedule:inline_scheduler;

import std;

import libfork.core;

namespace lf {

// TODO: think about initialization:
// - do we need default initializable on stack/context?
// - with allocators

// TODO: Can we store the context directly in TLS?

export template <worker_context Context>
class inline_scheduler {

  // TODO: how to adapt for polymorphic contexts?

 public:
  using context_type = Context;

  auto post(lf::sched_handle<Context> handle) { execute(m_context, handle); }

 private:
  Context m_context;
};

} // namespace lf
