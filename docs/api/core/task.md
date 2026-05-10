---
icon: lucide/box
---

# Task

```cpp
template <returnable T, worker_context Context>
class task {
 public:
  using value_type = T;
  using context_type = Context;
};
```

See associated:

- [returnable](./concepts.md#returnable)
- [worker_context](./concepts.md#worker-contexts)

The return type for all coroutines/async-functions in libfork. This type exists
so that users can mark their functions as coroutines. Other than its typedefs
it has no public interface.

!!! warning
    No consumer of this library should ever touch an instance of this type,
    it is used for specifying the return type of a coroutine only.

!!! example
    With a simple type alias, coroutines can be written more compactly:

    ```cpp
    template <lf::returnable T>
    using task = lf::task<T, some_worker_context>;
    ```

    Now you can write coroutines like this:

    ```cpp
    auto my_coroutine() -> task<int> {
      co_return 42;
    }
    ```

    See [env](env.md) for writing context-generic coroutines.
