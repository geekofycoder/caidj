#pragma once

// Portability shim for std::atomic<std::shared_ptr<T>> (C++20, P0718R2).
//
// libstdc++ (GCC 13, the toolchain used for the reported results) implements
// the specialisation. libc++ (Apple clang) does not, and rejects it with a
// "requires that 'T' be a trivially copyable type" static assertion.
//
// The fallback uses the std::atomic_load/std::atomic_store free functions,
// which provide the same atomicity guarantee for shared_ptr. They are
// deprecated in C++20 but still supplied by libc++.
//
// Memory ordering is passed through on the native path. The free functions
// are sequentially consistent, which is stronger than the acquire/release
// used by callers, so the fallback is never weaker.

#include <atomic>
#include <memory>

#if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
#define CAIDJ_NATIVE_ATOMIC_SHARED_PTR 1
#else
#define CAIDJ_NATIVE_ATOMIC_SHARED_PTR 0
#endif

namespace caidj::util
{

    template <class T>
    class AtomicSharedPtr
    {
    public:
        AtomicSharedPtr() = default;
        explicit AtomicSharedPtr(std::shared_ptr<T> p) : slot_(std::move(p)) {}

        std::shared_ptr<T> load(std::memory_order order = std::memory_order_seq_cst) const
        {
#if CAIDJ_NATIVE_ATOMIC_SHARED_PTR
            return slot_.load(order);
#else
            (void)order;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            return std::atomic_load(&slot_);
#pragma clang diagnostic pop
#endif
        }

        void store(std::shared_ptr<T> p, std::memory_order order = std::memory_order_seq_cst)
        {
#if CAIDJ_NATIVE_ATOMIC_SHARED_PTR
            slot_.store(std::move(p), order);
#else
            (void)order;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            std::atomic_store(&slot_, std::move(p));
#pragma clang diagnostic pop
#endif
        }

    private:
#if CAIDJ_NATIVE_ATOMIC_SHARED_PTR
        std::atomic<std::shared_ptr<T>> slot_;
#else
        std::shared_ptr<T> slot_;
#endif
    };

} // namespace caidj::util
