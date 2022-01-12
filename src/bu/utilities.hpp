#pragma once


// This file is intended to be included by every single translation unit in the project


#include "bu/disable_unnecessary_warnings.hpp"

#include <cstddef>
#include <cstdint>
#include <cassert>

#include <limits>
#include <utility>
#include <concepts>
#include <functional>
#include <type_traits>
#include <source_location>

#include <fstream>
#include <iostream>
#include <filesystem>

#include <exception>
#include <stdexcept>

#include <span>
#include <tuple>
#include <array>
#include <vector>
#include <variant>
#include <optional>

#include <format>
#include <string>
#include <string_view>

#include <ranges>
#include <numeric>
#include <algorithm>


namespace bu {

    using I8  = std::int8_t;
    using I16 = std::int16_t;
    using I32 = std::int32_t;
    using I64 = std::int64_t;

    using U8  = std::uint8_t;
    using U16 = std::uint16_t;
    using U32 = std::uint32_t;
    using U64 = std::uint64_t;

    using Usize = std::size_t;
    using Isize = std::make_signed_t<Usize>;

    using Char = unsigned char;

    using Float = double;


    inline namespace literals {

        using namespace std::literals;

        consteval auto operator"" _i8 (Usize n) noexcept -> I8  { return static_cast<I8 >(n); }
        consteval auto operator"" _i16(Usize n) noexcept -> I16 { return static_cast<I16>(n); }
        consteval auto operator"" _i32(Usize n) noexcept -> I32 { return static_cast<I32>(n); }
        consteval auto operator"" _i64(Usize n) noexcept -> I64 { return static_cast<I64>(n); }

        consteval auto operator"" _u8 (Usize n) noexcept -> U8  { return static_cast<U8 >(n); }
        consteval auto operator"" _u16(Usize n) noexcept -> U16 { return static_cast<U16>(n); }
        consteval auto operator"" _u32(Usize n) noexcept -> U32 { return static_cast<U32>(n); }
        consteval auto operator"" _u64(Usize n) noexcept -> U64 { return static_cast<U64>(n); }

        consteval auto operator"" _uz(Usize n) noexcept -> Usize { return static_cast<Usize>(n); }
        consteval auto operator"" _iz(Usize n) noexcept -> Isize { return static_cast<Isize>(n); }

    }


    template <std::ostream& os = std::cout>
    auto print(std::string_view fmt, auto const&... args) -> void {
        if constexpr (sizeof...(args) != 0) {
            os << std::format(fmt, args...);
        }
        else {
            os << fmt;
        }
    }

    [[noreturn]]
    inline auto abort(std::string_view message, std::source_location caller = std::source_location::current()) -> void {
        throw std::runtime_error(
            std::format(
                "bu::abort invoked in {}, in file {} on line {}, with message: {}",
                caller.function_name(),
                caller.file_name(),
                caller.line(),
                message
            )
        );
    }

    [[noreturn]]
    inline auto unimplemented(std::source_location caller = std::source_location::current()) -> void {
        abort("Unimplemented branch reached", caller);
    }


    template <class Fst, class Snd = Fst>
    struct [[nodiscard]] Pair {
        Fst first;
        Snd second;

        [[nodiscard]]
        constexpr auto operator==(Pair const&) const
            noexcept(noexcept(pair == pair, second == second)) -> bool = default;
    };

    inline constexpr auto first  = [](auto& pair) noexcept -> auto& { return pair.first ; };
    inline constexpr auto second = [](auto& pair) noexcept -> auto& { return pair.second; };


    inline constexpr auto move = []<class X>(X&& x) noexcept -> std::remove_reference_t<X>&& {
        static_assert(!std::is_const_v<std::remove_reference_t<X>>, "Attempted to move from const");
        return static_cast<std::remove_reference_t<X>&&>(x);
    };

    template <class T>
    inline constexpr auto make = []<class... Args>(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args&&...>) -> T
    {
        return T { std::forward<Args>(args)... };
    };


    [[nodiscard]]
    constexpr auto unsigned_distance(auto const start, auto const stop) noexcept -> Usize {
        return static_cast<Usize>(std::distance(start, stop));
    }


    struct Formatter_base {
        constexpr auto parse(std::format_parse_context& context) {
            return context.end();
        }
    };

}


#define DEFAULTED_EQUALITY(name) \
auto operator==(name const&) const noexcept -> bool = default


#define DECLARE_FORMATTER_FOR_TEMPLATE(...)                             \
struct std::formatter<__VA_ARGS__> : bu::Formatter_base {               \
    [[nodiscard]] auto format(__VA_ARGS__ const&, std::format_context&) \
        -> std::format_context::iterator;                               \
}

#define DECLARE_FORMATTER_FOR(...) \
template <> DECLARE_FORMATTER_FOR_TEMPLATE(__VA_ARGS__)

#define DEFINE_FORMATTER_FOR(...) \
auto std::formatter<__VA_ARGS__>::format(__VA_ARGS__ const& value, std::format_context& context) \
    -> std::format_context::iterator


template <class... Ts>
DECLARE_FORMATTER_FOR_TEMPLATE(std::variant<Ts...>);
template <class T>
DECLARE_FORMATTER_FOR_TEMPLATE(std::optional<T>);
template <class T>
DECLARE_FORMATTER_FOR_TEMPLATE(std::vector<T>);
template <class F, class S>
DECLARE_FORMATTER_FOR_TEMPLATE(bu::Pair<F, S>);


template <class... Ts>
DEFINE_FORMATTER_FOR(std::variant<Ts...>) {
    return std::visit([out = context.out()](auto const& alternative) {
        return std::format_to(out, "{}", alternative);
    }, value);
}

template <class T>
DEFINE_FORMATTER_FOR(std::optional<T>) {
    if (value) {
        return std::format_to(context.out(), "{}", *value);
    }
    else {
        return std::format_to(context.out(), "std::nullopt");
    }
}

template <class T>
DEFINE_FORMATTER_FOR(std::vector<T>) {
    auto out = context.out();

    if (value.empty()) {
        return std::format_to(out, "[]");
    }
    else {
        std::format_to(out, "[{}", value.front());
        for (auto& x : value | std::views::drop(1)) {
            std::format_to(out, ", {}", x);
        }
        return std::format_to(out, "]");
    }
}

template <class F, class S>
DEFINE_FORMATTER_FOR(bu::Pair<F, S>) {
    return std::format_to(context.out(), "({}, {})", value.first, value.second);
}


template <>
struct std::formatter<std::monostate> : bu::Formatter_base {
    auto format(std::monostate, std::format_context& context) {
        return std::format_to(context.out(), "std::monostate");
    }
};