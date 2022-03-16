#pragma once

#include "bu/utilities.hpp"


namespace bu {

    template <class T>
    class [[nodiscard]] Uninitialized {
        std::aligned_storage_t<sizeof(T), alignof(T)> buffer;
        bool is_initialized = false;
    public:
        Uninitialized() noexcept = default;

        Uninitialized(Uninitialized const&) = delete;
        Uninitialized(Uninitialized&&)      = delete;

        auto operator=(Uninitialized const&) = delete;
        auto operator=(Uninitialized&&)      = delete;

        ~Uninitialized()
            noexcept(std::is_nothrow_destructible_v<T>)
        {
            if (is_initialized) [[likely]] {
                reinterpret_cast<T const*>(&buffer)->~T();
            }
        }

        template <class... Args>
        auto initialize(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>) -> void
        {
            assert(!is_initialized);
            is_initialized = true;
            ::new(&buffer) T(std::forward<Args>(args)...);
        }

        decltype(auto) operator*(this auto&& self) noexcept {
            assert(self.is_initialized);
            return bu::forward_like<decltype(self)>(reinterpret_cast<T&>(self.buffer));
        }
    };

}