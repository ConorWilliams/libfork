---
icon: lucide/puzzle
---

# Concepts

The core module defines a large number of concepts and associated type traits.

## Returnable

```cpp
template <typename T>
concept returnable = std::is_void_v<T> || (/*plain-object*/<T> && std::movable<T>);
```

Types suitable for use as return types for libfork's [task](./task.md). See
below for the definition of `plain-object`.

### Plain-object

An exposition-only concept:

```cpp
template <typename T>
concept /*plain-object*/ = std::is_object_v<T> && std::same_as<T, std::remove_reference_t<T>>;
```

## Worker context

TODO: impl
