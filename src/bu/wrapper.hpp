#pragma once

#include "bu/utilities.hpp"


namespace bu {

    template <class T>
    class [[nodiscard]] Wrapper {
        static_assert(std::is_object_v<T>);

        static std::vector<std::remove_const_t<T>> vector;

        Usize index;
    public:
        template <class... Args>
            requires ((sizeof...(Args) != 1) || (!similar_to<Wrapper, Args> && ...))
        constexpr Wrapper(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<std::remove_const_t<T>, Args&&...>)
            : index { vector.size() }
        {
            vector.emplace_back(std::forward<Args>(args)...);
        }

        constexpr auto operator*(this Wrapper const self) noexcept -> T& {
            return vector[self.index];
        }

        constexpr auto operator->(this Wrapper const self) noexcept -> T* {
            return &*self;
        }

        constexpr operator T&(this Wrapper const self) noexcept {
            return *self;
        }

        auto hash(this Wrapper const self)
            noexcept(noexcept(std::hash<T>{}(*self))) -> Usize
        {
            return std::hash<T>{}(*self);
        }

        constexpr static auto release_wrapped_memory() noexcept {
            bu::release_vector_memory(vector);
        }
    };

    // Initializer lifted out of the class due to apparent compiler bug
    template <class T>
    std::vector<std::remove_const_t<T>> Wrapper<T>::vector =
        bu::vector_with_capacity<std::remove_const_t<T>>(256);

    template <class T>
    Wrapper(T) -> Wrapper<T>;


    template <class T>
    constexpr auto operator==(Wrapper<T> const a, Wrapper<T> const b)
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