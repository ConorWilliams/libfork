export module libfork.schedulers:inline_scheduler;

import std;

import libfork.core;

namespace lf {

// TODO: think about initialization:
// - do we need default initializable on stack/context?
// - with allocators

// TODO: Can we store the context directly in TLS?

export template <derived_worker_context Context>
class inline_scheduler {
 public:
  using context_type = Context::context_type;

  void post(lf::sched_handle<context_type> handle) {
    execute(static_cast<context_type &>(m_context), handle);
  }

 private:
  Context m_context;
};

} // namespace lf
