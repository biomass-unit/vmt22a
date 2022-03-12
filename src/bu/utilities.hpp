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
#include <list>
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

    static_assert(CHAR_BIT == 8);

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


    namespace dtl {

        inline auto filename_without_path(std::string_view path) noexcept -> std::string_view {
            auto const trim_if = [&](char const c) {
                if (auto const pos = path.find_last_of(c); pos != std::string_view::npos) {
                    path.remove_prefix(pos + 1);
                }
            };

            trim_if('\\');
            trim_if('/');

            return path;
        }

    }


    template <std::ostream& os = std::cout>
    auto print(std::string_view fmt, auto const&... args) -> void {
        if constexpr (sizeof...(args) != 0) {
            auto const buffer = std::format(fmt, args...);
            os.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        }
        else {
            os.write(fmt.data(), static_cast<std::streamsize>(fmt.size()));
        }
    }

    [[noreturn]]
    inline auto abort(std::string_view message, std::source_location caller = std::source_location::current()) -> void {
        throw std::runtime_error(
            std::format(
                "bu::abort invoked in {}, in file {} on line {}, with message: {}",
                caller.function_name(),
                dtl::filename_without_path(caller.file_name()),
                caller.line(),
                message
            )
        );
    }

    [[noreturn]]
    inline auto unimplemented(std::source_location caller = std::source_location::current()) -> void {
        abort("Unimplemented branch reached", caller);
    }

    inline auto trace(std::source_location caller = std::source_location::current()) -> void {
        print(
            "bu::trace: Reached line {} in {}, in function {}\n",
            caller.line(),
            dtl::filename_without_path(caller.file_name()),
            caller.function_name()
        );
    }


    template <class Fst, class Snd = Fst>
    struct [[nodiscard]] Pair {
        Fst first;
        Snd second;

        [[nodiscard]]
        constexpr auto operator==(Pair const&) const
            noexcept(noexcept(first == first, second == second)) -> bool = default;
    };

    constexpr auto first  = [](auto& pair) noexcept -> auto& { return pair.first ; };
    constexpr auto second = [](auto& pair) noexcept -> auto& { return pair.second; };


    constexpr auto move = []<class X>(X&& x) noexcept -> std::remove_reference_t<X>&& {
        static_assert(!std::is_const_v<std::remove_reference_t<X>>, "Attempted to move from const");
        return static_cast<std::remove_reference_t<X>&&>(x);
    };

    constexpr auto copy = []<std::copyable X>(X const& x)
        noexcept(std::is_nothrow_copy_constructible_v<X>) -> X
    {
        return x;
    };

    constexpr auto dereference = [](auto const& x)
        noexcept(noexcept(*x)) -> decltype(auto)
    {
        return *x;
    };

    template <class T>
    constexpr auto make = []<class... Args>(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args&&...>) -> T
    {
        return T { std::forward<Args>(args)... };
    };


    constexpr auto compose = []<class F, class G>(F&& f, G&& g) noexcept {
        return [f = std::forward<F>(f), g = std::forward<G>(g)]<class... Args>(Args&&... args)
            mutable noexcept(
                std::is_nothrow_invocable_v<G&&, Args&&...> &&
                std::is_nothrow_invocable_v<F&&, std::invoke_result_t<G&&, Args&&...>>
            ) -> decltype(auto)
        {
            return std::invoke(
                std::forward<F>(f),
                std::invoke(
                    std::forward<G>(g),
                    std::forward<Args>(args)...
                )
            );
        };
    };

    static_assert(
        compose(
            [](int x) { return x * x; },
            [](int a, int b) { return a + b; }
        )(2, 3) == 25
    );


    template <class... Fs>
    struct Overload : Fs... {
        using Fs::operator()...;
    };

    template <class... Fs>
    Overload(Fs...) -> Overload<Fs...>;


    template <class>
    struct Typetag {};
    template <class T>
    constexpr Typetag<T> typetag;

    template <auto>
    struct Value {};
    template <auto x>
    constexpr Value<x> value;


    template <class, class>
    constexpr Usize alternative_index;
    template <class... Ts, class T>
    constexpr Usize alternative_index<std::variant<Ts...>, T> =
        std::variant<Typetag<Ts>...> { Typetag<T> {} }.index();


    template <class T>
    /*constexpr*/ auto vector_with_capacity(Usize const capacity) noexcept -> std::vector<T> {
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
        assert(start <= stop);
        return static_cast<Usize>(std::distance(start, stop));
    }

    [[nodiscard]]
    constexpr auto digit_count(std::integral auto integer) noexcept -> Usize {
        Usize digits = 0;
        do {
            integer /= 10;
            ++digits;
        }
        while (integer);
        return digits;
    }

    static_assert(digit_count(0) == 1);
    static_assert(digit_count(-10) == 2);
    static_assert(digit_count(-999) == 3);
    static_assert(digit_count(12345) == 5);


    namespace dtl {

        template <class, template <class...> class>
        struct Is_instance_of : std::false_type {};
        template <class... Args, template <class...> class F>
        struct Is_instance_of<F<Args...>, F> : std::true_type {};

        template <class T, class U>
        using Copy_reference = std::conditional_t<
            std::is_rvalue_reference_v<T>,
            std::remove_reference_t<U>&&,
            U&
        >;

        template <class T, class U>
        using Copy_const = std::conditional_t<
            std::is_const_v<std::remove_reference_t<T>>,
            U const,
            U
        >;

    }


    template <class T, class U>
    using Like = dtl::Copy_reference<T&&, dtl::Copy_const<T, std::remove_reference_t<U>>>;

    template <class T>
    constexpr auto forward_like(auto&& x) noexcept -> Like<T, decltype(x)> {
        return static_cast<Like<T, decltype(x)>>(x);
    }

    // ^^^ These are only necessary until the major compilers start supporting C++23


    template <class T, template <class...> class F>
    concept instance_of = dtl::Is_instance_of<T, F>::value;

    template <class T, class U>
    concept similar_to = std::same_as<std::decay_t<T>, std::decay_t<U>>;

    template <class T>
    concept trivial = std::is_trivial_v<T>;

    template <class>
    constexpr bool always_false = false;


    constexpr auto map = []<class Optional, class F>(Optional&& optional, F&& f)
        noexcept(std::is_nothrow_invocable_v<F&&, decltype(forward_like<Optional>(*optional))>)
        requires instance_of<std::decay_t<Optional>, std::optional>
    {
        if (optional) {
            return std::optional { std::invoke(std::move(f), forward_like<Optional>(*optional)) };
        }
        else {
            return decltype(std::optional { std::invoke(std::move(f), forward_like<Optional>(*optional)) }) {};
        }
    };


    auto serialize_to(std::output_iterator<std::byte> auto out, trivial auto... args)
        noexcept -> void
    {
        ([=]() mutable noexcept {
            auto const memory = reinterpret_cast<std::byte const*>(&args);
            for (Usize i = 0; i != sizeof args; ++i) {
                *out++ = memory[i];
            }
        }(), ...);
    }


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

#define DIRECTLY_DEFINE_FORMATTER_FOR(...) \
DECLARE_FORMATTER_FOR(__VA_ARGS__);        \
DEFINE_FORMATTER_FOR(__VA_ARGS__)


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
    return value
        ? std::format_to(context.out(), "{}", *value)
        : std::format_to(context.out(), "std::nullopt");
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