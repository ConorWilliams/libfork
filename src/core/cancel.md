# Cancel notes

Goals:

- Symmetry between schedule and fork/call cancellation binding
- Allocator aware schedule (using shared pointer std:: function)
- Customize the construction of receiver
- Default schedule should be non-cancellable (bind nullptr)
- Join should be a member function of the scope
- Cancel scope instead of separate source+scope

## Task 1 - cancel scope

```cpp
auto example() -> task<void> {

  auto sc = co_await child_scope();

  co_await sc.fork(fn1, 0);  
  co_await sc.call(fn2, sc.token());

  co_await sc.join();
}
```

The result of `sc.token()` should be `stop_token` a lightweight wrapper around
a pointer to a stop source that has the stop_requested and other member
functions.

You can convert the current `stop_source` to a simple internal-only struct and
convert uses of `stop_source*` to use `stop_token`.

Make `join()` a member of both the regular and child scope's via a shared base
class.

## Task 2 - Schedule API

The receiver class and state should have a cancellable template parameter.

The receiver state should have public constructors which forwards arguments for
in-place construction of the return value. The rest of the members should
become private.

The API of `schedule` should be something like:

```cpp
  requires decay_invocable_to<Fn, R, Context, Args...>
auto schedule(std::shared_ptr<receiver_state<R, Cancellable>> recv_state, Fn && fn, Args&&... args...)
```

This allows users to customize the allocation if desired. A convenience
overload (which delegated to above) should exist, which just allocates via
`make_shared`. It should default to non-cancellable.
