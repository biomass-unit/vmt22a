#pragma once

#include "bu/utilities.hpp"


namespace bu {

    template <class>
    class [[nodiscard]] Wrapper_context;


    template <class T>
    class [[nodiscard]] Wrapper {
        Usize index;

        friend class Wrapper_context<T>;

        inline static std::vector<T>* vector = nullptr;
        inline static Wrapper_context<T> default_context { 0 };
    public:
        template <class... Args>
        constexpr explicit Wrapper(Args&&... args) noexcept(std::is_nothrow_constructible_v<T>)
            : index { vector->size() }
        {
            vector->emplace_back(std::forward<Args>(args)...);
        }

        constexpr explicit(false) operator T const&() const noexcept { return vector->operator[](index); }
        constexpr explicit(false) operator T      &()       noexcept { return vector->operator[](index); }

        constexpr auto operator*() const noexcept -> T const& { return *this; }
        constexpr auto operator*()       noexcept -> T      & { return *this; }

        constexpr auto operator->() const noexcept -> T const* { return vector->data() + index; }
        constexpr auto operator->()       noexcept -> T      * { return vector->data() + index; }
    };

    template <class T>
    Wrapper(T) -> Wrapper<T>;


    template <class T>
    class [[nodiscard]] Wrapper_context {
        std::vector<T>  vector;
        std::vector<T>* old_vector;
    public:
        constexpr explicit Wrapper_context(Usize const initial_capacity = 256) noexcept
            : old_vector { std::exchange(Wrapper<T>::vector, &vector) }
        {
            vector.reserve(initial_capacity);
        }

        Wrapper_context(Wrapper_context const&) = delete;

        constexpr ~Wrapper_context() {
            Wrapper<T>::vector = old_vector;
        }
    };


    template <class A, std::equality_comparable_with<A> B>
    constexpr auto operator==(Wrapper<A> const a, Wrapper<B> const b)
        noexcept(noexcept(*a == *b)) -> bool
    {
        return *a == *b;
    }

}


template <class T>
struct std::formatter<bu::Wrapper<T>> : std::formatter<T> {
    auto format(bu::Wrapper<T> const wrapper, std::format_context& context) {
        return std::formatter<T>::format(*wrapper, context);
    }
};