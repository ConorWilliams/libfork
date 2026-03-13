export module libfork.core:schedule;

import std;

import :concepts;
import :frame;

namespace lf {

// schedule(context*, fn, args...) -> heap allocated shared state this must
// create a task, bind approprate values + cancellation submit the task to a
// context. So a context must have a submit(...) method, the thing we submit
// must be atomicable, ideally it would be able to accpet submission at either
// a root OR context switch point

struct block_deleter {
  static void operator()(block_type *block) noexcept { release_ref(block); }
};

template <std::default_initializable T>
struct block;

export template <typename T>
auto make_block() noexcept -> std::unique_ptr<block<T>, block_deleter> {
  return std::unique_ptr<block<T>, block_deleter>{new block<T>{}};
}

template <std::default_initializable T>
struct block : block_type {

  T return_value;

 private:
  constexpr block() = default;

  friend auto make_block<T>() noexcept -> std::unique_ptr<block<T>, block_deleter>;
};

export template <worker_context Context, typename... Args, async_invocable<Args...> Fn>
constexpr auto schedule(Context *context, Fn &&fn, Args &&...args) noexcept -> void {
  ///
}

} // namespace lf
