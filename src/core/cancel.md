# Cancel notes

Goals:

- Symmetry between schedule and fork/call cancellation binding ✓
- Allocator aware schedule (using shared pointer std:: function) ✓
- Customize the construction of receiver ✓
- Default schedule should be non-cancellable (bind nullptr) ✓
- Join should be a member function of the scope ✓
- Cancel scope instead of separate source+scope ✓

## Task 1 - cancel scope ✓

```cpp
auto example() -> task<void> {

  auto sc = co_await child_scope();

  co_await sc.fork(fn1, 0);  
  co_await sc.call(fn2, sc.token());

  co_await sc.join();
}
```

`child_scope()` returns a `child_scope_ops<Context>` that:
- Owns a `stop_source` chained onto the parent frame's cancel token.
- All `fork`/`call` operations automatically bind the scope's stop source
  as the child's cancel source (Cancel=true path).
- `.token()` returns a `stop_token` wrapping the scope's stop source.
- `.join()` is available via the shared `scope_base` base class
  (same as calling `co_await lf::join()`).

`stop_source` is now internal-only (not exported). The public API is
`stop_token` — a lightweight pointer-sized wrapper that exposes
`stop_requested()`, `request_stop()`, and `race_request_stop()`.

`scope_ops` (obtained via `co_await lf::scope()`) also inherits `scope_base`
and therefore also exposes `.join()`. For explicit cancel binding from a
regular scope, `fork_with(token, ...)` / `call_with(token, ...)` accept a
`stop_token`.

## Task 2 - Schedule API ✓

`receiver_state<T, Stoppable=false>` has:
- Public default constructor + forwarding constructors for in-place T construction.
- All other members private, with accessor methods used by `root_pkg` and `schedule`.
- A `stop_source` member only when `Stoppable=true`.
- `get_stop_token()` (requires `Stoppable=true`) returns a `stop_token`.

`receiver<T, Stoppable=false>` exposes:
- `token()` (requires `Stoppable=true`) for external cancellation.

`schedule` has two overloads:

```cpp
// Primary: caller supplies a pre-allocated (possibly custom-allocated) state.
auto schedule(Sch&&, shared_ptr<receiver_state<R, Stoppable>>, Fn&&, Args&&...)
    -> receiver<R, Stoppable>;

// Convenience: allocates via make_shared, non-cancellable by default.
auto schedule(Sch&&, Fn&&, Args&&...)
    -> receiver<R>;   // receiver<R, false>
```
