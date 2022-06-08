#pragma once

#include "bu/utilities.hpp"


namespace bu {

    template <class T>
    class [[nodiscard]] Wrapper {
        static_assert(std::is_object_v<T>);
        static_assert(!std::is_const_v<T>);

        static auto vector() noexcept -> std::vector<T>&; // Avoids SIOF

        Usize index;
    public:
        template <class... Args>
            requires ((sizeof...(Args) != 1) || (!similar_to<Wrapper, Args> && ...))
        Wrapper(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>)
            : index { vector().size() }
        {
            vector().emplace_back(std::forward<Args>(args)...);
        }

        auto operator*(this Wrapper const self) noexcept -> T& {
            return vector()[self.index];
        }

        auto operator->(this Wrapper const self) noexcept -> T* {
            return std::addressof(*self);
        }

        operator T&(this Wrapper const self) noexcept {
            return *self;
        }

        auto hash(this Wrapper const self)
            noexcept(noexcept(::bu::hash(*self))) -> Usize
            requires hashable<T>
        {
            return ::bu::hash(*self);
        }

        static auto release_wrapped_memory() noexcept -> void {
            release_vector_memory(vector());
        }
    };


    template <class T>
    auto Wrapper<T>::vector() noexcept -> std::vector<T>& {
        static auto vec = vector_with_capacity<T>(1024);
        return vec;
    }


    template <class T>
    Wrapper(T) -> Wrapper<T>;


    template <class T>
    auto operator==(Wrapper<T> const a, Wrapper<T> const b)
        noexcept(noexcept(*a == *b)) -> bool
    {
        return *a == *b;
    }

    constexpr auto wrap = []<class X>(X&& x) {
        return Wrapper<std::decay_t<X>> { std::forward<X>(x) };
    };


    template <class T>
    concept wrapper = bu::instance_of<T, Wrapper>;

}


template <class T>
struct std::formatter<bu::Wrapper<T>> : std::formatter<T> {
    auto format(bu::Wrapper<T> const wrapper, std::format_context& context) {
        return std::formatter<T>::format(*wrapper, context);
    }
};