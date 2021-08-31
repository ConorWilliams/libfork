#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

// This (stand-alone) file implements the deque described in the papers, "Correct and Efficient
// Work-Stealing for Weak Memory Models," and "Dynamic Circular Work-Stealing Deque". Both are
// available in 'reference/'.

namespace riften {

namespace detail {

// Basic wrapper around a c-style array of atomic objects that provides modulo load/stores. Capacity
// must be a power of 2.
template <typename T> struct RingBuff {
  public:
    explicit RingBuff(std::int64_t cap) : _cap{cap}, _mask{cap - 1} {
        assert(cap && (!(cap & (cap - 1))) && "Capacity must be buf power of 2!");
    }

    std::int64_t capacity() const noexcept { return _cap; }

    // Relaxed store at modulo index
    void store(std::int64_t i, T const x) noexcept { _buff[i & _mask].store(x, std::memory_order_relaxed); }

    // Relaxed load at modulo index
    T load(std::int64_t i) const noexcept { return _buff[i & _mask].load(std::memory_order_relaxed); }

    // Allocates and returns a new ring buffer, copies elements in range [b, t) into the new buffer.
    RingBuff<T>* resize(std::int64_t b, std::int64_t t) const {
        RingBuff<T>* ptr = new RingBuff{2 * _cap};
        for (std::int64_t i = t; i != b; ++i) {
            ptr->store(i, load(i));
        }
        return ptr;
    }

  private:
    std::int64_t _cap;   // Capacity of the buffer
    std::int64_t _mask;  // Bit mask to perform modulo capacity operations

    std::unique_ptr<std::atomic<T>[]> _buff = std::make_unique_for_overwrite<std::atomic<T>[]>(_cap);
};

}  // namespace detail

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_constructive_interference_size;
using std::hardware_destructive_interference_size;
#else
// 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
inline constexpr std::size_t hardware_constructive_interference_size = 2 * sizeof(std::max_align_t);
inline constexpr std::size_t hardware_destructive_interference_size = 2 * sizeof(std::max_align_t);
#endif

// Lock-free single-producer multiple-consumer deque. Only the deque owner can perform pop and push
// operations where the deque behaves like a stack. Others can (only) steal data from the deque, they see
// a FIFO queue. All threads must have finished using the deque before it is destructed.
template <typename T> class Deque2 {
  private:
    static_assert(std::atomic<T>::is_always_lock_free && std::is_trivially_destructible_v<T>, "");

  public:
    // Constructs the deque with a given capacity the capacity of the deque (must be power of 2)
    explicit Deque2(std::int64_t cap = 1024);

    // Move/Copy is not supported
    Deque2(Deque2 const& other) = delete;
    Deque2& operator=(Deque2 const& other) = delete;

    //  Query the size at instance of call
    std::size_t size() const noexcept;

    // Query the capacity at instance of call
    int64_t capacity() const noexcept;

    // Test if empty at instance of call
    bool empty() const noexcept;

    // Emplace an item to the deque. Only the owner thread can insert an item to the deque. The
    // operation can trigger the deque to resize its cap if more space is required. Provides the
    // strong exception guarantee.
    template <typename... Args> void emplace(Args&&... args);

    // Pops out an item from the deque. Only the owner thread can pop out an item from the deque.
    // The return can be a std::nullopt if this operation fails (empty deque).
    std::optional<T> pop() noexcept;

    // Steals an item from the deque Any threads can try to steal an item from the deque. The return
    // can be a std::nullopt if this operation failed (not necessarily empty).
    std::optional<T> steal() noexcept;

    // Destruct the deque, all threads must have finished using the deque.
    ~Deque2() noexcept;

  private:
    alignas(hardware_destructive_interference_size) std::atomic<std::int64_t> _top;
    alignas(hardware_destructive_interference_size) std::atomic<std::int64_t> _bottom;
    alignas(hardware_destructive_interference_size) std::atomic<detail::RingBuff<T>*> _buffer;

    std::vector<std::unique_ptr<detail::RingBuff<T>>> _garbage;  // Store old buffers here.

    // Convenience aliases.
    static constexpr std::memory_order relaxed = std::memory_order_relaxed;
    static constexpr std::memory_order consume = std::memory_order_consume;
    static constexpr std::memory_order acquire = std::memory_order_acquire;
    static constexpr std::memory_order release = std::memory_order_release;
    static constexpr std::memory_order seq_cst = std::memory_order_seq_cst;
};

template <typename T> Deque2<T>::Deque2(std::int64_t cap)
    : _top(0), _bottom(0), _buffer(new detail::RingBuff<T>{cap}) {
    _garbage.reserve(32);
}

template <typename T> std::size_t Deque2<T>::size() const noexcept {
    int64_t b = _bottom.load(relaxed);
    int64_t t = _top.load(relaxed);
    return static_cast<std::size_t>(b >= t ? b - t : 0);
}

template <typename T> int64_t Deque2<T>::capacity() const noexcept {
    return _buffer.load(relaxed)->capacity();
}

template <typename T> bool Deque2<T>::empty() const noexcept { return !size(); }

template <typename T> template <typename... Args> void Deque2<T>::emplace(Args&&... args) {
    // Construct before acquiring slot in-case constructor throws
    T object(std::forward<Args>(args)...);

    std::int64_t b = _bottom.load(relaxed);
    std::int64_t t = _top.load(acquire);
    detail::RingBuff<T>* buf = _buffer.load(relaxed);

    if (buf->capacity() < (b - t) + 1) {
        // Queue is full, build a new one
        _garbage.emplace_back(std::exchange(buf, buf->resize(b, t)));
        _buffer.store(buf, relaxed);
    }

    // Construct new object
    buf->store(b, object);

    std::atomic_thread_fence(release);
    _bottom.store(b + 1, relaxed);
}

template <typename T> std::optional<T> Deque2<T>::pop() noexcept {
    std::int64_t b = _bottom.load(relaxed) - 1;
    detail::RingBuff<T>* buf = _buffer.load(relaxed);
    _bottom.store(b, relaxed);
    std::atomic_thread_fence(seq_cst);
    std::int64_t t = _top.load(relaxed);

    if (t <= b) {
        // Non-empty deque
        if (t == b) {
            // The last item could get stolen
            if (!_top.compare_exchange_strong(t, t + 1, seq_cst, relaxed)) {
                // Failed race.
                _bottom.store(b + 1, relaxed);
                return std::nullopt;
            }
            _bottom.store(b + 1, relaxed);
        }

        // Can delay load until after acquiring slot as only this thread can push()
        return buf->load(b);

    } else {
        _bottom.store(b + 1, relaxed);
        return std::nullopt;
    }
}

template <typename T> std::optional<T> Deque2<T>::steal() noexcept {
    std::int64_t t = _top.load(acquire);
    std::atomic_thread_fence(seq_cst);
    std::int64_t b = _bottom.load(acquire);

    if (t < b) {
        // Must load *before* acquiring the slot as slot may be overwritten immediately after
        // acquiring.
        T x = _buffer.load(consume)->load(t);

        if (!_top.compare_exchange_strong(t, t + 1, seq_cst, relaxed)) {
            // Failed race.
            return std::nullopt;
        }

        return x;

    } else {
        // Empty deque.
        return std::nullopt;
    }
}

template <typename T> Deque2<T>::~Deque2() noexcept { delete _buffer.load(); }

}  // namespace riften