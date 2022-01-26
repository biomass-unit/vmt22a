#pragma once

#include "bu/utilities.hpp"


namespace bu {

    template <class T>
    class [[nodiscard]] Uninitialized {
        std::aligned_storage_t<sizeof(T), alignof(T)> buffer;

#ifndef NDEBUG
        bool is_initialized = false;
#endif
    public:
        Uninitialized() noexcept = default;

        Uninitialized(Uninitialized const&) = delete;
        Uninitialized(Uninitialized&&)      = delete;

        auto operator=(Uninitialized const&) = delete;
        auto operator=(Uninitialized&&)      = delete;

        ~Uninitialized()
            noexcept(std::is_nothrow_destructible_v<T>)
        {
#ifndef NDEBUG
            assert(is_initialized);
#endif
            reinterpret_cast<T const*>(&buffer)->~T();
        }

        template <class... Args>
        auto initialize(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>) -> void
        {
#ifndef NDEBUG
            assert(!is_initialized);
            is_initialized = true;
#endif
            ::new(&buffer) T(std::forward<Args>(args)...);
        }

        auto read_initialized() const noexcept -> T const& {
#ifndef NDEBUG
            assert(is_initialized);
#endif
            return *reinterpret_cast<T const*>(&buffer);
        }
        auto read_initialized() noexcept -> T& {
            return const_cast<T&>(const_cast<Uninitialized const*>(this)->read_initialized());
        }
    };

}