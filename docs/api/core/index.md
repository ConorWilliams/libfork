---
icon: lucide/blocks
---

# Core

All public symbols documented here live in namespace `lf` and are reachable via:

```cpp
import libfork.core;
```

The module is contains the minimal set of components for using libfork. Notably
this excludes concrete schedulers and stacks. Use just this module if your
writing your own and don't want the smallest dependency possible.

Bits:

- scope and child_scope
- exception derivation

TODO:

```fish
rg -Pn --glob '*.{cpp,cxx,cc,h,hpp,hxx,ixx}' '\bexport\b(?!\s+(module|import)\b)' src/core/

src/core/exception.cxx
10:export struct libfork_exception : std::exception {};

src/core/concepts/indirect.cxx
41:export template <typename Fn, typename I>
56:export template <typename Fn, typename I>
75:export template <typename Fn, typename Context, typename I>
91:export template <typename Fn, typename Context, typename I>
112:export template <typename Fn, typename Context, typename I>
125:export template <typename Fn, typename Context, typename I>

src/core/concepts/context.cxx
18:export template <typename T, typename U>
32:export template <typename T>
40:export template <worker_context T>

src/core/concepts/semigroup.cxx
53:export template <typename Fn, typename I>
105:export template <typename Fn, typename Context, typename I>
123:export template <typename Fn, typename Context, typename I>
131:export template <typename Fn, typename Context, typename I>
150:export template <typename Fn, typename Context, typename I>

src/core/concepts/stack.cxx
37:export template <typename T>
49:// export template <typename T>

src/core/execute.cxx
16:export struct execute_error final : libfork_exception {
31:export template <worker_context Context>
62:export struct steal_overflow_error final : libfork_exception {
102:export template <worker_context Context>

src/core/schedule.cxx
22:export struct schedule_error final : libfork_exception {
45:export template <scheduler Sch, typename R, bool Stoppable, decay_copyable Fn, decay_copyable... Args>
103:export template <scheduler Sch, decay_copyable Fn, decay_copyable... Args>

src/core/root.cxx
22:export struct root_alloc_error final : libfork_exception {

src/core/stop.cxx
12:export class stop_source {

src/core/concepts/scheduler.cxx
9:export template <typename T>
12:export template <has_context_typedef T>
23:export template <typename Sch>

src/core/concepts/invocable.cxx
33:export template <typename Fn, typename Context, typename... Args>
42:export template <typename Fn, typename Context, typename... Args>
48:export template <typename Fn, typename Context, typename... Args>
55:export template <typename Fn, typename Context, typename... Args>
62:export template <typename Fn, typename R, typename Context, typename... Args>
69:export template <typename Fn, typename R, typename Context, typename... Args>

src/core/concepts/awaitable.cxx
43:export template <awaitable_acquirable T>
80:export template <typename T, typename Context>

src/core/task.cxx
16:export template <typename T>
19:export template <worker_context>
44:export template <returnable T, worker_context Context>

src/core/receiver.cxx
14:export struct broken_receiver_error final : libfork_exception {
21:export struct operation_cancelled_error final : libfork_exception {
75:export template <typename T, bool Stoppable = false>
117:export template <typename T, bool Stoppable = false>

src/core/projected.cxx
77:export template <typename Fn, typename Context, typename I>
84:export template <worker_context Context, std::indirectly_readable I, projectable<Context, I> Fn>

src/core/handles.cxx
33:export struct unsafe_steal_handle : handle {
42:export struct unsafe_sched_handle : handle {
55:export template <typename T>
67:export template <typename T>

src/core/poly_context.cxx
13:export template <worker_stack Stack>
31:export struct post_error final : libfork_exception {
45:export template <worker_stack Stack>
```
