#pragma once


// This file is intended to be included by every single translation unit in the project


#include "disable_unnecessary_warnings.hpp"

#include <cstddef>
#include <cstdint>
#include <cassert>

#include <limits>
#include <memory>
#include <utility>
#include <iostream>
#include <concepts>
#include <functional>
#include <type_traits>
#include <source_location>

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
            noexcept(noexcept(first == first, second == second)) -> bool = default;
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


    template <class T, std::invocable<T&&> F>
    constexpr auto map(std::optional<T>&& optional, F&& f)
        noexcept(std::is_nothrow_invocable_v<F&&, T&&>)
        -> std::optional<std::invoke_result_t<F&&, T&&>>
    {
        if (optional) {
            return std::invoke(std::move(f), std::move(*optional));
        }
        else {
            return std::nullopt;
        }
    }


    template <class T>
    constexpr auto vector_with_capacity(Usize const capacity) noexcept -> std::vector<T> {
        std::vector<T> vector;
        vector.reserve(capacity);
        return vector;
    }

    inline constexpr auto string_without_sso() noexcept -> std::string {
        std::string string;
        string.reserve(sizeof string);
        return string;
    }

    [[nodiscard]]
    constexpr auto unsigned_distance(auto const start, auto const stop) noexcept -> Usize {
        return static_cast<Usize>(std::distance(start, stop));
    }

    [[nodiscard]]
    inline constexpr auto to_pointers(std::string_view const view) noexcept -> Pair<char const*> {
        return { view.data(), view.data() + view.size() };
    }


    namespace dtl {
        template <class, template <class...> class>
        struct Is_instance_of : std::false_type {};
        template <class... Args, template <class...> class F>
        struct Is_instance_of<F<Args...>, F> : std::true_type {};
    }

    template <class T, template <class...> class F>
    concept instance_of = dtl::Is_instance_of<T, F>::value;

    template <class T, class U>
    concept similar_to = std::same_as<std::decay_t<T>, std::decay_t<U>>;

    template <class T>
    concept trivial = std::is_trivial_v<T>;


    template <Usize length>
    struct [[nodiscard]] Metastring {
        char string[length];

        consteval Metastring(char const* pointer) noexcept {
            std::copy_n(pointer, length, string);
        }

        constexpr auto view() const noexcept -> std::string_view {
            return { string, length };
        }
    };

    template <Usize length>
    Metastring(char const(&)[length]) -> Metastring<length - 1>;


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
    if (!value.empty()) {
        std::format_to(out, "{}", value.front());
        for (auto& x : value | std::views::drop(1)) {
            std::format_to(out, ", {}", x);
        }
    }
    return out;
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