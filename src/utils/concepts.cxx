module;
#include "libfork/__impl/utils.hpp"
export module libfork.utils:concepts;

import std;

namespace lf {

// =========== Atomic related concepts =========== //

export template <typename T>
concept plain_object = std::is_object_v<T> && std::same_as<T, std::remove_reference_t<T>>;

/**
 * @brief Verify a type is suitable for use with `std::atomic`
 *
 * This requires a `TriviallyCopyable` type satisfying both `CopyConstructible` and `CopyAssignable`.
 */
export template <typename T>
concept atomicable = plain_object<T> &&                 //
                     std::is_trivially_copyable_v<T> && //
                     std::is_copy_constructible_v<T> && //
                     std::is_move_constructible_v<T> && //
                     std::is_copy_assignable_v<T> &&    //
                     std::is_move_assignable_v<T>;      //

/**
 * @brief A concept that verifies a type is lock-free when used with `std::atomic`.
 */
export template <typename T>
concept lock_free = atomicable<T> && std::atomic<T>::is_always_lock_free;

// ========== Specialization ========== //

template <typename T, template <typename...> typename Template>
struct is_specialization_of : std::false_type {};

template <template <typename...> typename Template, typename... Args>
struct is_specialization_of<Template<Args...>, Template> : std::true_type {};

/**
 * @brief Test if `T` is a specialization of the template `Template`.
 */
export template <typename T, template <typename...> typename Template>
concept specialization_of = is_specialization_of<std::remove_cvref_t<T>, Template>::value;

} // namespace lf
