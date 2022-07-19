#pragma once

#include "bu/utilities.hpp"


namespace bu {

    constexpr struct Unchecked {} unchecked;

    struct Safe_integer_out_of_range : std::exception {
        auto what() const -> char const* override {
            return "bu::Safe_integer out of range";
        }
    };

    struct Safe_integer_overflow : Safe_integer_out_of_range {
        auto what() const -> char const* override {
            return "bu::Safe_integer arithmetic operation overflowed";
        }
    };

    struct Safe_integer_underflow : Safe_integer_out_of_range {
        auto what() const -> char const* override {
            return "bu::Safe_integer arithmetic operation underflowed";
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


    template <std::integral T>
    class Safe_integer {
        T value;

        template <class U = T>
        static constexpr auto is_in_range(std::integral auto const x) noexcept -> bool {
            return std::cmp_less_equal(std::numeric_limits<U>::min(), x)
                && std::cmp_less_equal(x, std::numeric_limits<U>::max());
        }
    public:
        constexpr Safe_integer() noexcept
            : value {} {}

        constexpr explicit Safe_integer(Unchecked, std::integral auto const value) noexcept
            : value { static_cast<T>(value) }
        {
            assert(is_in_range(value));
        }

        constexpr Safe_integer(std::integral auto const value)
            : value { static_cast<T>(value) }
        {
            if (!is_in_range(value)) [[unlikely]] {
                throw Safe_integer_out_of_range {};
            }
        }

        constexpr auto get(this Safe_integer const self) noexcept -> T {
            return self.value;
        }


        constexpr auto operator+=(Safe_integer const other) -> Safe_integer& {
            if (would_addition_overflow(value, other.value)) [[unlikely]] {
                throw Safe_integer_overflow {};
            }
            if (would_addition_underflow(value, other.value)) [[unlikely]] {
                throw Safe_integer_underflow {};
            }

            value += other.value;
            return *this;
        }

        constexpr auto operator+(this Safe_integer self, Safe_integer const other)
            -> Safe_integer
        {
            return self += other;
        }


        constexpr auto operator-=(Safe_integer const other) -> Safe_integer& {
            if (would_subtraction_overflow(value, other.value)) [[unlikely]] {
                throw Safe_integer_overflow {};
            }
            if (would_subtraction_underflow(value, other.value)) [[unlikely]] {
                throw Safe_integer_underflow {};
            }

            value -= other.value;
            return *this;
        }

        constexpr auto operator-(this Safe_integer self, Safe_integer const other)
            -> Safe_integer
        {
            return self -= other;
        }


        constexpr auto operator*=(Safe_integer const other) -> Safe_integer& {
            if (would_multiplication_overflow(value, other.value)) [[unlikely]] {
                throw Safe_integer_overflow {};
            }
            if (would_multiplication_underflow(value, other.value)) [[unlikely]] {
                throw Safe_integer_underflow {};
            }

            value *= other.value;
            return *this;
        }

        constexpr auto operator*(this Safe_integer self, Safe_integer const other)
            -> Safe_integer
        {
            return self *= other;
        }


        constexpr auto operator/=(Safe_integer const other) -> Safe_integer& {
            if (other.get() == 0) [[unlikely]] {
                throw exception("bu::Safe_integer division by zero");
            }

            if (would_division_overflow(value, other.value)) [[unlikely]] {
                throw Safe_integer_overflow {};
            }

            value /= other.value;
            return *this;
        }

        constexpr auto operator/(this Safe_integer self, Safe_integer const other)
            -> Safe_integer
        {
            return self /= other;
        }


        constexpr auto operator%=(Safe_integer const other) -> Safe_integer& {
            if (would_division_overflow(value, other.value)) [[unlikely]] {
                throw Safe_integer_overflow {};
            }

            value %= other.value;
            return *this;
        }

        constexpr auto operator%(this Safe_integer self, Safe_integer const other)
            -> Safe_integer
        {
            return self %= other;
        }


        constexpr auto operator++() -> Safe_integer& {
            if (would_increment_overflow(value)) [[unlikely]] {
                throw Safe_integer_overflow {};
            }

            ++value;
            return *this;
        }

        constexpr auto operator++(int) -> Safe_integer {
            auto copy = *this;
            ++*this;
            return copy;
        }


        constexpr auto operator--() -> Safe_integer& {
            if (would_decrement_underflow(value)) [[unlikely]] {
                throw Safe_integer_underflow {};
            }

            --value;
            return *this;
        }

        constexpr auto operator--(int) -> Safe_integer {
            auto copy = *this;
            ++*this;
            return copy;
        }


        constexpr explicit operator bool(this Safe_integer const self) noexcept {
            return self.value != 0;
        }


        auto operator== (Safe_integer const&) const noexcept -> bool                 = default;
        auto operator<=>(Safe_integer const&) const noexcept -> std::strong_ordering = default;
    };


    using Safe_i8  = Safe_integer<I8 >;
    using Safe_i16 = Safe_integer<I16>;
    using Safe_i32 = Safe_integer<I32>;
    using Safe_i64 = Safe_integer<I64>;

    using Safe_u8  = Safe_integer<U8 >;
    using Safe_u16 = Safe_integer<U16>;
    using Safe_u32 = Safe_integer<U32>;
    using Safe_u64 = Safe_integer<U64>;

    using Safe_usize = Safe_integer<Usize>;
    using Safe_isize = Safe_integer<Isize>;

}


template <class T>
struct std::formatter<bu::Safe_integer<T>> : std::formatter<T> {
    auto format(bu::Safe_integer<T> const value, std::format_context& context) {
        return std::formatter<T>::format(value.get(), context);
    }
};