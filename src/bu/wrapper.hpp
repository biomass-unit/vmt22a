#pragma once

#include "bu/utilities.hpp"


namespace bu {

    template <class T>
    class [[nodiscard]] Wrapper {
        static std::vector<T> vector;

        Usize index;
    public:
        template <class... Args> // The constraint ensures that the copy constructor isn't deleted
            requires ((sizeof...(Args) != 1) || (!similar_to<Wrapper, Args> && ...))
        constexpr Wrapper(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>)
            : index { vector.size() }
        {
            vector.emplace_back(std::forward<Args>(args)...);
        }

        constexpr explicit(false) operator T const&() const noexcept { return vector[index]; }
        constexpr explicit(false) operator T      &()       noexcept { return vector[index]; }

        constexpr auto operator*() const noexcept -> T const& { return *this; }
        constexpr auto operator*()       noexcept -> T      & { return *this; }

        constexpr auto operator->() const noexcept -> T const* { return vector.data() + index; }
        constexpr auto operator->()       noexcept -> T      * { return vector.data() + index; }

        constexpr static auto release_wrapped_memory() noexcept {
            bu::release_vector_memory(vector);
        }
    };

    // Initializer lifted out of the class due to apparent compiler bug
    template <class T>
    std::vector<T> Wrapper<T>::vector = bu::vector_with_capacity<T>(256);

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

template <class T>
struct std::hash<bu::Wrapper<T>> : std::hash<T> {
    auto operator()(bu::Wrapper<T> const wrapper) -> bu::Usize {
        return std::hash<T>::operator()(*wrapper);
    }
};