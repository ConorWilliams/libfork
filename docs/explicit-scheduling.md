# Explicit Scheduling

Explicit scheduling lets a running task hand its continuation to a
user-supplied awaitable, which is free to enqueue the task on a
different worker / context before the current thread returns to the
fork-join machinery. It is the mechanism behind "context switching"
primitives (e.g. moving onto an I/O thread, a NUMA-pinned worker, a
priority queue, …) without breaking the strict fork-join invariants.

## User contract

A type `T` is awaitable inside a libfork task when it satisfies
`lf::awaitable<T, Context>`. The concept requires that, after running
`acquire_awaitable` (which honours member or free `operator co_await`),
the resulting type exposes:

- `await_ready() -> bool`
- `await_suspend(lf::sched_handle<Context>, Context&) -> void`
- `await_resume() -> ...`

The `sched_handle` is opaque; the awaitable's only job is to make sure
some thread eventually calls `lf::execute(ctx, handle)` on it. Once
`await_suspend` returns, the original thread is no longer the owner of
that task.

Concept and `acquire_awaitable` plumbing live in
`src/core/concepts/awaitable.cxx`. The user-facing concept is
`lf::awaitable`; the dispatch helper is `acquire_awaitable`.

## How `co_await` is wired

`mixin_frame::await_transform` in `src/core/promise.cxx` adds an
overload constrained on `awaitable<Context>`. It first runs
`acquire_awaitable` (so member/free `operator co_await` is honoured),
then constructs a `switch_awaitable<Context, T>` from the result. The
ordinary `pkg<…>` (fork/call) and `join` / `scope` overloads remain
unchanged.

## `switch_awaitable`

Defined in `src/core/awaitables.cxx`. On suspension it:

1. Computes `owns_stack = parent.steals == 0` — same lemma as the
   join-race winner.
2. Calls `context.stack().prepare_release()` *before* invoking the
   user's `await_suspend`, because once the handle has been published
   we can no longer touch the parent frame.
3. Invokes `value.await_suspend(sched_handle{key, &parent.frame}, ctx)`.
   If this throws, `await_suspend` propagates and the parent is resumed
   with the exception live — `prepare_release` is reversible and
   leaves the stack untouched.
4. If we owned the stack, releases it via the prepared key; otherwise
   discards the key (our stack is already empty by the steal lemma).
5. Returns `resume_effectively_stolen(context)` — see below — wrapped
   in a `noexcept` IIFE: at this point we have already given the task
   away and cannot resume the parent, so an exception here is
   unrecoverable and must terminate.

## "Effectively stolen" tasks and lost joins

Once explicit scheduling has run, a worker's WSQ may contain ancestor
tasks that morally belong to whichever thread later resumes the
moved-out task. `resume_effectively_stolen` (in
`src/core/awaitables.cxx`) pops one such task and routes it through
`consume` (extracted in `src/core/execute.cxx`) so the steal counter
is incremented exactly as if a remote worker had taken it. The cases
are documented inline; the short version:

- **Win the join in the ancestor** → safe to keep treating remaining
  WSQ entries as non-stolen.
- **Lose the join** → recurse: the lost-join continuation calls
  `resume_effectively_stolen` again, draining one more entry.
- **Lose the join in a descendant** → every remaining WSQ entry has
  already been taken by another worker; `pop()` returns null and the
  helper yields `std::noop_coroutine()`.

The lost-join branch in `join_awaitable::await_suspend`
(`src/core/awaitables.cxx`) now returns
`resume_effectively_stolen(get_tls_context<Context>())` in place of
the old `noop_coroutine()` placeholder, completing the loop.

## `execute` and `consume`

`src/core/execute.cxx` exports two `execute` overloads:

- `execute(Context&, sched_handle<Context>)` — runs a task that was
  parked at a context switch (or has not yet started). The handle
  must be non-null.
- `execute(Context&, steal_handle<Context>)` — runs a task that was
  taken off another worker's WSQ. Internally calls `consume`, which
  bumps `frame->steals` and throws `steal_overflow_error` if the
  counter would saturate. `consume` is also reused by
  `resume_effectively_stolen`.

## Stack handling summary

`switch_awaitable` and `final_suspend_full`
(`src/core/final_suspend.cxx`) share the same release protocol:
`prepare_release` first, observe the race outcome, then either
`release(std::move(key))` if we owned the stack or drop the key. The
`slab_stack` change in `src/batteries/slab_stack.cxx` (storing
`m_init_size` instead of reading it back off the released slab) makes
the post-release slab size deterministic regardless of how the stack
was handed off.

## Related files at a glance

| File | Role |
| --- | --- |
| `src/core/concepts/awaitable.cxx` | `awaitable` concept, `acquire_awaitable` dispatch |
| `src/core/awaitables.cxx` | `switch_awaitable`, `resume_effectively_stolen`, lost-join wiring |
| `src/core/promise.cxx` | `await_transform` overload that builds `switch_awaitable` |
| `src/core/execute.cxx` | `execute` overloads, `consume` |
| `src/core/handles.cxx` | `sched_handle` / `steal_handle` types |
| `src/core/final_suspend.cxx` | Sibling stack-release protocol |
| `src/batteries/slab_stack.cxx` | Deterministic post-release slab sizing |
| `test/src/concepts.cpp` | Concept-level tests for `acquire_awaitable` and `awaitable` |
