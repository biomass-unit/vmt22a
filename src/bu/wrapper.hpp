#pragma once

#include "bu/utilities.hpp"


namespace bu {

    template <class T>
    class [[nodiscard]] Wrapper {
        inline static auto vector = bu::vector_with_capacity<T>(256);

        Usize index;
    public:
        template <class... Args> // The constraint ensures that the copy constructor isn't deleted
            requires ((sizeof...(Args) != 1) || (!similar_to<Wrapper, Args> && ...))
        constexpr Wrapper(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args&&...>)
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

        static constexpr auto object_count() noexcept -> Usize {
            return vector.size();
        }
    };

    template <class T>
    Wrapper(T) -> Wrapper<T>;


    template <class T>
    constexpr auto operator==(Wrapper<T> const a, Wrapper<T> const b)
        noexcept(noexcept(*a == *b)) -> bool
    {
        return *a == *b;
    }

    template <class T>
    constexpr auto make_wrapper = make<Wrapper<T>>;

}


template <class T>
struct std::formatter<bu::Wrapper<T>> : std::formatter<T> {
    auto format(bu::Wrapper<T> const wrapper, std::format_context& context) {
        return std::formatter<T>::format(*wrapper, context);
    }
};