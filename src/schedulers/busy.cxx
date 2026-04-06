export module libfork.schedulers:busy_scheduler;

import std;

import libfork.core;
import libfork.batteries;

namespace lf {

export template <bool Polymorphic, worker_stack Stack>
class busy_scheduler {

  using context = ; // TODO: derived_poly or mono_context

 public:
  using context_type = ; // TODO:

  void post(lf::sched_handle<context_type> handle) {
    execute(static_cast<context_type &>(m_context), handle);
  }

 private:
  context m_context;
};

} // namespace lf
