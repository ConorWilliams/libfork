---
icon: lucide/earth
---

# Env

```cpp
template <worker_context>
struct env {}
```

See associated:

- [worker_context](./concepts.md#worker-context)

A tag type that can be used to help write context-generic code. This can be
used as the (templated) first parameter of a coroutine such that
[worker_context](./concepts.md#worker-context) can be deduced. This parameter
will be generated and passed to the coroutine by libfork automatically.

!!! note
    This type is not user-constructible.

!!! example
    For example:

    ```cpp
    struct context_generic {
      template <typename Context>
      auto operator()(lf::env<Context>, int param) -> lf::task<void, Context> {
        // ...
      }
    }
    ```

    Then this can be called from any context:

    ```cpp linenums="1"
    auto some_coro(/* args */) -> lf::task<void, some_context> {
      // ...
      co_await lf::invoke(context_generic{}, 42);
      // ...
    }
    ```

    !!! note
        Here we defined `context_generic` as a function object so that we could
        pass it to [lf::invoke](./invoke.md) without needing to specify template
        parameters.
