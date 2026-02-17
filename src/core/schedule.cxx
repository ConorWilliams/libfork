export module libfork.core:schedule;

import std;

namespace lf {

// schedule(context*, fn, args...) -> heap allocated shared state this must
// create a task, bind approprate values + cancellation submit the task to a
// context. So a context must have a submit(...) method, the thing we submit
// must be atomicable, ideally it would be able to accpet submission at either
// a root OR context switch point

} // namespace lf
