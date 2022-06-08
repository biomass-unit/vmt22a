#pragma once

#include "bu/utilities.hpp"


namespace bu {

    constexpr struct Unchecked {} unchecked;

    struct Bounded_integer_out_of_range : std::exception {
        auto what() const -> char const* override {
            return "bu::Bounded_integer out of range";
        }
    };

    struct Bounded_integer_overflow : Bounded_integer_out_of_range {
        auto what() const -> char const* override {
            return "bu::Bounded_integer arithmetic operation overflowed";
        }
    };

    struct Bounded_integer_underflow : Bounded_integer_out_of_range {
        auto what() const -> char const* override {
            return "bu::Bounded_integer arithmetic operation underflowed";
        }
    };


    // Overflow & underflow detection logic for addition, subtraction, multiplication, and division
    // taken from https://vladris.com/blog/2018/10/13/arithmetic-overflow-and-underflow.html


    template <std::integral T>
    constexpr auto would_addition_overflow(T const a, T const b) noexcept -> bool {
        return (b >= 0) && (a > std::numeric_limits<T>::max() - b);
    }

    template <std::integral T>
    constexpr auto would_addition_underflow(T const a, T const b) noexcept -> bool {
        return (b < 0) && (a < std::numeric_limits<T>::min() - b);
    }


    template <std::integral T>
    constexpr auto would_subtraction_overflow(T const a, T const b) noexcept -> bool {
        return (b < 0) && (a > std::numeric_limits<T>::max() + b);
    }

    template <std::integral T>
    constexpr auto would_subtraction_underflow(T const a, T const b) noexcept -> bool {
        return (b >= 0) && (a < std::numeric_limits<T>::min() + b);
    }


    template <std::integral T>
    constexpr auto would_multiplication_overflow(T const a, T const b) noexcept -> bool {
        return b != 0
            ? ((b > 0) && (a > 0) && (a > std::numeric_limits<T>::max() / b)) ||
              ((b < 0) && (a < 0) && (a < std::numeric_limits<T>::max() / b))
            : false;
    }

    template <std::integral T>
    constexpr auto would_multiplication_underflow(T const a, T const b) noexcept -> bool {
        return b != 0
            ? ((b > 0) && (a < 0) && (a < std::numeric_limits<T>::min() / b)) ||
              ((b < 0) && (a > 0) && (a > std::numeric_limits<T>::min() / b))
            : false;
    }


    template <std::integral T>
    constexpr auto would_division_overflow(T const a, T const b) noexcept -> bool {
        return (a == std::numeric_limits<T>::min()) && (b == -1)
            && (a != 0);
    }


    template <std::integral X>
    constexpr auto would_increment_overflow(X const x) noexcept -> bool {
        return x == std::numeric_limits<X>::max();
    }

    template <std::integral X>
    constexpr auto would_decrement_underflow(X const x) noexcept -> bool {
        return x == std::numeric_limits<X>::min();
    }


    template <
        std::integral T,
        T min = std::numeric_limits<T>::min(),
        T max = std::numeric_limits<T>::max(),
        T default_value = (std::cmp_less_equal(min, 0) && std::cmp_less_equal(0, max))
            ? 0 // If zero is in range then it is the default-default-value, otherwise it must be explicitly specified
            : throw "please specify a default value"
    >
    class Bounded_integer {
        static_assert((min <= default_value) && (default_value <= max));

        T value;

        static constexpr auto is_in_range(std::integral auto const x) noexcept -> bool {
            return std::cmp_less_equal(min, x) && std::cmp_less_equal(x, max);
        }
    public:
        constexpr Bounded_integer() noexcept
            : value { default_value } {}

        constexpr explicit Bounded_integer(Unchecked, std::integral auto const value) noexcept
            : value { static_cast<T>(value) }
        {
            assert(is_in_range(value));
        }

        constexpr Bounded_integer(std::integral auto const value)
            : value { static_cast<T>(value) }
        {
            if (!is_in_range(value)) [[unlikely]] {
                throw Bounded_integer_out_of_range {};
            }
        }

        constexpr auto get(this Bounded_integer const self) noexcept -> T {
            return self.value;
        }

        template <std::integral U>
        constexpr auto safe_cast(this Bounded_integer const self) noexcept -> U {
            static_assert(std::cmp_greater_equal(min, std::numeric_limits<U>::min())
                       && std::cmp_less_equal   (max, std::numeric_limits<U>::max()));
            return static_cast<U>(self.value);
        }


        constexpr auto operator+=(Bounded_integer const other) -> Bounded_integer& {
            if (would_addition_overflow(value, other.value)) [[unlikely]] {
                throw Bounded_integer_overflow {};
            }
            if (would_addition_underflow(value, other.value)) [[unlikely]] {
                throw Bounded_integer_underflow {};
            }

            value += other.value;
            return *this;
        }

        constexpr auto operator+(this Bounded_integer self, Bounded_integer const other)
            -> Bounded_integer
        {
            return self += other;
        }


        constexpr auto operator-=(Bounded_integer const other) -> Bounded_integer& {
            if (would_subtraction_overflow(value, other.value)) [[unlikely]] {
                throw Bounded_integer_overflow {};
            }
            if (would_subtraction_underflow(value, other.value)) [[unlikely]] {
                throw Bounded_integer_underflow {};
            }

            value -= other.value;
            return *this;
        }

        constexpr auto operator-(this Bounded_integer self, Bounded_integer const other)
            -> Bounded_integer
        {
            return self -= other;
        }


        constexpr auto operator*=(Bounded_integer const other) -> Bounded_integer& {
            if (would_multiplication_overflow(value, other.value)) [[unlikely]] {
                throw Bounded_integer_overflow {};
            }
            if (would_multiplication_underflow(value, other.value)) [[unlikely]] {
                throw Bounded_integer_underflow {};
            }

            value *= other.value;
            return *this;
        }

        constexpr auto operator*(this Bounded_integer self, Bounded_integer const other)
            -> Bounded_integer
        {
            return self *= other;
        }


        constexpr auto operator/=(Bounded_integer const other) -> Bounded_integer& {
            if (would_division_overflow(value, other.value)) [[unlikely]] {
                throw Bounded_integer_overflow {};
            }

            value /= other.value;
            return *this;
        }

        constexpr auto operator/(this Bounded_integer self, Bounded_integer const other)
            -> Bounded_integer
        {
            return self /= other;
        }


        constexpr auto operator%=(Bounded_integer const other) -> Bounded_integer& {
            if (would_division_overflow(value, other.value)) [[unlikely]] {
                throw Bounded_integer_overflow {};
            }

            value %= other.value;
            return *this;
        }

        constexpr auto operator%(this Bounded_integer self, Bounded_integer const other)
            -> Bounded_integer
        {
            return self %= other;
        }


        constexpr auto operator++() -> Bounded_integer& {
            if (would_increment_overflow(value)) [[unlikely]] {
                throw Bounded_integer_overflow {};
            }

            ++value;
            return *this;
        }

        constexpr auto operator++(int) -> Bounded_integer {
            auto copy = *this;
            ++*this;
            return copy;
        }


        constexpr auto operator--() -> Bounded_integer& {
            if (would_decrement_underflow(value)) [[unlikely]] {
                throw Bounded_integer_underflow {};
            }

            --value;
            return *this;
        }

        constexpr auto operator--(int) -> Bounded_integer {
            auto copy = *this;
            ++*this;
            return copy;
        }


        constexpr explicit operator bool(this Bounded_integer const self) noexcept {
            return value != 0;
        }


        auto operator== (Bounded_integer const&) const noexcept -> bool                 = default;
        auto operator<=>(Bounded_integer const&) const noexcept -> std::strong_ordering = default;
    };


    using Bounded_i8  = Bounded_integer<I8 >;
    using Bounded_i16 = Bounded_integer<I16>;
    using Bounded_i32 = Bounded_integer<I32>;
    using Bounded_i64 = Bounded_integer<I64>;

    using Bounded_u8  = Bounded_integer<U8 >;
    using Bounded_u16 = Bounded_integer<U16>;
    using Bounded_u32 = Bounded_integer<U32>;
    using Bounded_u64 = Bounded_integer<U64>;

    using Bounded_usize = Bounded_integer<Usize>;
    using Bounded_isize = Bounded_integer<Isize>;

}


template <class T, T... config>
struct std::formatter<bu::Bounded_integer<T, config...>> : std::formatter<T> {
    auto format(bu::Bounded_integer<T, config...> const value, std::format_context& context) {
        return std::formatter<T>::format(value.get(), context);
    }
};