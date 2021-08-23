#pragma once

#include <coroutine>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace riften {

namespace detail {

// Test for co_await non-member overload
template <typename T>
concept non_member_co = requires {
    operator co_await(std::declval<T>());
};

// Test for co_await member overload
template <typename T>
concept member_co = requires {
    std::declval<T>().operator co_await();
};

// Base case, for (true, true) as ambiguous overload set
template <typename T, bool mem, bool non> struct get_awaiter : std::false_type {};

template <typename T> struct get_awaiter<T, true, false> : std::true_type {
    using type = decltype(std::declval<T>().operator co_await());
};

template <typename T> struct get_awaiter<T, false, true> : std::true_type {
    using type = decltype(operator co_await(std::declval<T>()));
};

// General case just return type
template <typename T> struct get_awaiter<T, false, false> : std::true_type { using type = T; };

///////////////////////////////////////////////////////////////////////////////////////////////

// Fetch the awaiter obtained by (co_await T); either T or operator co_await
template <typename T> using awaiter_t = get_awaiter<T, member_co<T>, non_member_co<T>>::type;

// Gets the return type of (co_await T).await_suspend(...)
template <typename T>
requires requires { typename awaiter_t<T>; }
struct awaitor_suspend {
    using type = decltype(std::declval<awaiter_t<T>>().await_suspend(std::coroutine_handle<>{}));
};

// Tag type
struct any {};

}  // namespace detail

// Concept to verify Awaitable can be co_await'ed in a coroutine -> Result
template <typename T, typename Result = detail::any>
concept awaitable = requires(detail::awaiter_t<T> x) {
    { x.await_ready() } -> std::convertible_to<bool>;

    x.await_suspend(std::coroutine_handle<>{});

    typename std::enable_if<std::disjunction_v<
        std::is_same<typename detail::awaitor_suspend<T>::type, void>,
        std::is_same<typename detail::awaitor_suspend<T>::type, bool>,
        std::is_convertible<typename detail::awaitor_suspend<T>::type, std::coroutine_handle<>>

        >>;

    x.await_resume();

    typename std::enable_if<
        std::is_same_v<Result, detail::any> || std::convertible_to<decltype(x.await_resume()), Result>>;
};

template <awaitable T> using await_result_t = decltype(std::declval<detail::awaiter_t<T>>().await_resume());

}  // namespace riften