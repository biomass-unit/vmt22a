#pragma once

#include "bu/utilities.hpp"


namespace bu {

    constexpr struct Unchecked_tag {} unchecked_tag;

    struct Bounded_integer_out_of_range : std::exception {
        auto what() const -> char const* override {
            return "bu::Bounded_integer out of range";
        }
    };


    template <
        std::integral T,
        T min = std::numeric_limits<T>::min(),
        T max = std::numeric_limits<T>::max(),
        T default_value = (std::cmp_less_equal(min, 0) && std::cmp_less_equal(0, max))
            ? 0 // If zero is in range then it is the default-default
            : throw "please specify a default value"
    >
    class Bounded_integer {
        static_assert(min < max);

        T value;

        static constexpr auto is_in_range(T const x) noexcept -> bool {
            return std::cmp_less_equal(min, x) && std::cmp_less_equal(x, max);
        }
    public:
        constexpr Bounded_integer() noexcept
            : value { default_value } {}

        constexpr explicit Bounded_integer(Unchecked_tag, T value) noexcept
            : value { value }
        {
            assert(is_in_range(value));
        }

        constexpr explicit Bounded_integer(T const value)
            : value { value }
        {
            if (!is_in_range(value)) [[unlikely]] {
                throw Bounded_integer_out_of_range {};
            }
        }

        constexpr auto get() const noexcept -> T { return value; }

        constexpr auto copy() const noexcept -> Bounded_integer { return *this; }

        template <std::integral U>
        constexpr auto safe_cast() const noexcept -> U {
            static_assert(std::cmp_greater_equal(min, std::numeric_limits<U>::min())
                       && std::cmp_less_equal   (max, std::numeric_limits<U>::max()));
            return static_cast<U>(value);
        }

        constexpr auto safe_add(T const rhs) -> Bounded_integer& {
            // https://stackoverflow.com/questions/3944505/detecting-signed-overflow-in-c-c

            if (value >= 0) {
                if ((max - value) < rhs) {
                    throw Bounded_integer_out_of_range {};
                }
            }
            else if (rhs < (min - value)) {
                throw Bounded_integer_out_of_range {};
            }

            value += rhs;
            return *this;
        }

        template <class U, U rhs_min, U rhs_max, U rhs_def>
            requires (std::cmp_less_equal(min, rhs_min) && std::cmp_less_equal(rhs_max, max))
        constexpr auto safe_add(Bounded_integer<U, rhs_min, rhs_max, rhs_def> const rhs) -> Bounded_integer& {
            return safe_add(static_cast<T>(rhs.get()));
        }

        constexpr auto safe_mul(T const rhs) -> Bounded_integer& {
            if (!value || !rhs) [[unlikely]] {
                value = 0;
                return *this;
            }

            // https://www.cplusplus.com/articles/DE18T05o/

            static_assert(std::is_unsigned_v<T>, "signed multiplication is still a bit buggy");

            static constexpr auto maximum = std::is_unsigned_v<T>
                ? (max - min)
                : (max - min) / 2;

            if (value > (maximum / rhs)) {
                throw Bounded_integer_out_of_range {};
            }
            else {
                value *= rhs;
                return *this;
            }
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
struct std::formatter<bu::Bounded_integer<T, config...>> : bu::Formatter_base {
    auto format(bu::Bounded_integer<T, config...> const value, std::format_context& context) {
        return std::format_to(context.out(), "{}", value.get());
    }
};