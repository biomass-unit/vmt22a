#pragma once


// This file is intended to be included by every single translation unit in the project


#include "disable_unnecessary_warnings.hpp"

#include <cstddef>
#include <cstdint>
#include <cassert>

#include <limits>
#include <memory>
#include <utility>
#include <typeinfo>
#include <concepts>
#include <exception>
#include <functional>
#include <type_traits>
#include <source_location>

#include <fstream>
#include <iostream>
#include <filesystem>

#include <span>
#include <list>
#include <tuple>
#include <array>
#include <vector>
#include <variant>
#include <optional>
#include <expected>

#include <string>
#include <string_view>

#include <format>
#include <charconv>

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

    using Char = char;

    using Float = double;


    namespace dtl {

        template <class, template <class...> class>
        struct Is_instance_of : std::false_type {};
        template <class... Ts, template <class...> class F>
        struct Is_instance_of<F<Ts...>, F> : std::true_type {};

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

    template <class T, class... Ts>
    concept one_of = std::disjunction_v<std::is_same<T, Ts>...>;

    template <class T>
    concept trivial = std::is_trivial_v<T>;

    template <class T>
    concept trivially_copyable = std::is_trivially_copyable_v<T>;

    template <class>
    constexpr bool always_false = false;


    constexpr bool compiling_in_debug_mode =
#ifdef NDEBUG
        false;
#else
        true;
#endif

    constexpr bool compiling_in_release_mode = !compiling_in_debug_mode;


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

        consteval auto operator"" _format(char const* const s, Usize const n) noexcept {
            return [fmt = std::string_view { s, n }](auto const&... args) -> std::string {
                return std::vformat(fmt, std::make_format_args(args...));
            };
        }

    }


    namespace dtl {

        constexpr auto filename_without_path(std::string_view path) noexcept -> std::string_view {
            auto const trim_if = [&](char const c) {
                if (auto const pos = path.find_last_of(c); pos != std::string_view::npos) {
                    path.remove_prefix(pos + 1);
                }
            };

            trim_if('\\');
            trim_if('/');

            return path;
        }

        static_assert(filename_without_path("aaa/bbb/ccc")   == "ccc");
        static_assert(filename_without_path("aaa\\bbb\\ccc") == "ccc");

    }


    class [[nodiscard]] Exception : public std::exception {
        std::string message;
    public:
        Exception(std::string&& message) noexcept
            : message { std::move(message) } {}

        auto what() const -> char const* override {
            return message.c_str();
        }
    };


    template <std::ostream& os = std::cout>
    auto print(std::string_view const fmt, auto const&... args) -> void {
        if constexpr (sizeof...(args) != 0) {
            auto const buffer = std::vformat(fmt, std::make_format_args(args...));
            os.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        }
        else {
            os.write(fmt.data(), static_cast<std::streamsize>(fmt.size()));
        }
    }

    [[noreturn]]
    inline auto abort(
        std::string_view     const message,
        std::source_location const caller = std::source_location::current()) -> void
    {
        print(
            "bu::abort invoked in {}, in file {} on line {}, with message: {}",
            caller.function_name(),
            dtl::filename_without_path(caller.file_name()),
            caller.line(),
            message
        );
        std::terminate();
    }

    [[noreturn]]
    inline auto abort(
        std::source_location const caller = std::source_location::current()) -> void
    {
        abort("no message", caller);
    }

    inline auto always_assert(
        bool                 const assertion,
        std::source_location const caller = std::source_location::current()) -> void
    {
        if (!assertion) [[unlikely]] {
            abort("Assertion failed", caller);
        }
    }

    [[noreturn]]
    inline auto todo(
        std::source_location const caller = std::source_location::current()) -> void
    {
        abort("Unimplemented branch reached", caller);
    }

    inline auto trace(
        std::source_location const caller = std::source_location::current()) -> void
    {
        print(
            "bu::trace: Reached line {} in {}, in function {}\n",
            caller.line(),
            dtl::filename_without_path(caller.file_name()),
            caller.function_name()
        );
    }


    auto exception(std::string_view const fmt, auto const&... args) -> Exception {
        return Exception { std::vformat(fmt, std::make_format_args(args...)) };
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

    constexpr auto address = [](auto& x)
        noexcept -> auto*
    {
        return std::addressof(x);
    };

    constexpr auto dereference = [](auto const& x)
        noexcept(noexcept(*x)) -> decltype(auto)
    {
        return *x;
    };

    constexpr auto size = [](auto const& x) noexcept -> Usize {
        return std::size(x);
    };

    template <class T>
    constexpr auto make = []<class... Args>(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args&&...>) -> T
    {
        return T(std::forward<Args>(args)...);
    };


    template <class F, class G, class... Hs> [[nodiscard]]
    constexpr auto compose(F&& f, G&& g, Hs&&... hs) noexcept {
        if constexpr (sizeof...(Hs) != 0) {
            return compose(std::forward<F>(f), compose(std::forward<G>(g), std::forward<Hs>(hs)...));
        }
        else {
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
        }
    };

    static_assert(
        compose(
            [](int x) { return x * x; },
            [](int x) { return x + 1; },
            [](int a, int b) { return a + b; }
        )(2, 3) == 36
    );


    template <class... Fs>
    struct Overload : Fs... {
        using Fs::operator()...;
    };

    template <class... Fs>
    Overload(Fs...) -> Overload<Fs...>;


    template <class>
    struct Type {};
    template <class T>
    constexpr Type<T> type;


    template <auto>
    struct Value {};
    template <auto x>
    constexpr Value<x> value;


    template <class Variant, class Alternative>
    constexpr Usize alternative_index;
    template <class... Ts, class T>
    constexpr Usize alternative_index<std::variant<Ts...>, T> =
        std::variant<Type<Ts>...> { Type<T> {} }.index();


    class Hole {
        std::source_location caller;

        constexpr Hole(std::source_location const caller = std::source_location::current()) noexcept
            : caller { caller } {}

        friend constexpr auto hole(std::source_location) noexcept -> Hole;
    public:
        template <class T>
        constexpr operator T() {
            abort("Reached a hole of type {}"_format(typeid(T).name()), caller);
        }
    };

    constexpr auto hole(std::source_location const caller = std::source_location::current())
        noexcept -> Hole
    {
        return { caller };
    }


    template <class T, class V> [[nodiscard]]
    constexpr decltype(auto) get(
        V&& variant,
        std::source_location const caller = std::source_location::current()) noexcept
        requires requires { std::get_if<T>(&variant); }
    {
        if (T* const alternative = std::get_if<T>(&variant)) [[likely]] {
            return forward_like<V>(*alternative);
        }
        else [[unlikely]] {
            abort("Bad variant access", caller);
        }
    }

    template <class O> requires instance_of<std::decay_t<O>, std::optional> [[nodiscard]]
    constexpr decltype(auto) get(
        O&& optional,
        std::source_location const caller = std::source_location::current()) noexcept
    {
        if (optional.has_value()) [[likely]] {
            return forward_like<O>(*optional);
        }
        else [[unlikely]] {
            abort("Bad optional access", caller);
        }
    }

    template <class Ok, std::derived_from<std::exception> Err>
    constexpr auto expect(std::expected<Ok, Err>&& expected) -> Ok&& {
        if (expected) {
            return std::move(*expected);
        }
        else {
            throw std::move(expected.error());
        }
    }


    template <class T>
    auto vector_with_capacity(Usize const capacity) noexcept -> std::vector<T> {
        std::vector<T> vector;
        vector.reserve(capacity);
        return vector;
    }

    template <class T>
    auto release_vector_memory(std::vector<T>& vector) noexcept -> void {
        std::vector<T> {}.swap(vector);
    }

    template <class T, Usize n>
    auto vector_from(T(&&array)[n])
        noexcept(std::is_nothrow_move_constructible_v<T>) -> std::vector<T>
    {
        std::vector<T> output;
        output.reserve(n);
        output.insert(
            output.end(),
            std::make_move_iterator(std::begin(array)),
            std::make_move_iterator(std::end(array))
        );
        return output;
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
        template <class F>
        struct Mapper {
            F f;

            template <class T>
            auto operator()(this Mapper const self, std::vector<T>&& vector) {
                std::vector<std::decay_t<std::invoke_result_t<F const&, T&&>>> output;
                output.reserve(vector.size());
                for (T& element : vector) {
                    output.push_back(std::invoke(self.f, std::move(element)));
                }
                return output;
            }
            template <class T>
            auto operator()(this Mapper const self, std::vector<T> const& vector) {
                std::vector<std::decay_t<std::invoke_result_t<F&&, T const&>>> output;
                output.reserve(vector.size());
                for (T const& element : vector) {
                    output.push_back(std::invoke(self.f, element));
                }
                return output;
            }
        };
    }

    template <class F>
    auto map(F&& f) -> dtl::Mapper<std::decay_t<F>> {
        return { .f = std::forward<F>(f) };
    }


    template <class X>
    auto hash(X const& x)
        noexcept(std::is_nothrow_invocable_v<std::hash<X>, X const&>) -> Usize
        requires requires { { std::hash<X>{}(x) } -> std::same_as<Usize>; }
    {
        static_assert(std::is_empty_v<std::hash<X>>);
        return std::hash<X>{}(x);
    }

    template <class X>
    auto hash(X const& x)
        noexcept(noexcept(x.hash())) -> Usize
        requires requires { { x.hash() } -> std::same_as<Usize>; }
    {
        return x.hash();
    }

    template <class X>
    auto hash(X const&) -> void {
        static_assert(always_false<X>, "bu::hash: type not hashable");
    }

    template <class X>
    concept hashable = requires (X const x) {
        { ::bu::hash(x) } -> std::same_as<Usize>;
    };


    template <hashable Head, hashable... Tail>
    auto hash_combine_with_seed(
        Usize          seed,
        Head const&    head,
        Tail const&... tail) -> Usize
    {
        // https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x

        seed ^= (::bu::hash(head) + 0x9e3779b9 + (seed << 6) + (seed >> 2));

        if constexpr (sizeof...(Tail) != 0) {
            return hash_combine_with_seed<Tail...>(seed, tail...);
        }
        else {
            return seed;
        }
    }

    template <hashable... Args>
    auto hash_combine(Args const&... args) -> Usize {
        return hash_combine_with_seed(0, args...);
    }


    inline auto local_time() -> std::chrono::local_time<std::chrono::system_clock::duration> {
        return std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
    }


    consteval auto get_unique_seed(std::source_location const caller = std::source_location::current())
        noexcept -> bu::Usize
    {
        return (std::string_view { caller.file_name() }.size()
              * std::string_view { caller.function_name() }.size()
              + static_cast<Usize>(caller.line())
              * static_cast<Usize>(caller.column()));
    }

    static_assert(get_unique_seed() != get_unique_seed());


    auto serialize_to(std::output_iterator<std::byte> auto out, trivial auto const... args)
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

        consteval auto view() const noexcept -> std::string_view {
            return { string, length - 1 };
        }
    };

    template <Usize length>
    Metastring(char const(&)[length]) -> Metastring<length>;


    namespace fmt {

        struct Formatter_base {
            constexpr auto parse(std::format_parse_context& context)
                -> std::format_parse_context::iterator
            {
                assert(context.begin() == context.end());
                return context.end();
            }
        };

        struct Visitor_base {
            std::format_context::iterator out;

            auto format(std::string_view const fmt, auto const&... args) {
                return std::vformat_to(out, fmt, std::make_format_args(args...));
            }
        };


        template <std::integral T>
        struct Integer_with_ordinal_indicator_formatter_closure {
            T integer;
        };

        template <std::integral T>
        constexpr auto integer_with_ordinal_indicator(T const n) noexcept
            -> Integer_with_ordinal_indicator_formatter_closure<T>
        {
            return { .integer = n };
        }


        template <std::ranges::sized_range Range>
        struct Range_formatter_closure {
            Range const*     range;
            std::string_view delimiter;
        };

        template <std::ranges::sized_range Range>
        auto delimited_range(Range const& range, std::string_view const delimiter)
            -> Range_formatter_closure<Range>
        {
            return { &range, delimiter };
        }

    }


    namespace ranges {

        // Makeshift std::ranges::to, remove when C++23 is supported

        namespace dtl {
            template <class>
            struct To_direct {};

            template <template <class...> class>
            struct To_deduced {};

            template <std::ranges::range R, class Target>
            constexpr auto operator|(R&& range, To_direct<Target>) {
                return Target(
                    std::make_move_iterator(std::ranges::begin(range)),
                    std::make_move_iterator(std::ranges::end(range))
                );
            }

            template <std::ranges::range R, template <class...> class Target>
            constexpr auto operator|(R&& range, To_deduced<Target>) {
                return std::forward<R>(range) | move_to<Target<std::ranges::range_value_t<R>>>();
            }
        }

        template <class Target>
        constexpr auto move_to() -> dtl::To_direct<Target> { return {}; }

        template <template <class...> class Target>
        constexpr auto move_to() -> dtl::To_deduced<Target> { return {}; }


        // Makeshift std::ranges::contains, remove when C++23 is supported

        constexpr auto contains(std::ranges::range auto&& range, auto const& value) -> bool {
            return std::ranges::find(range, value) != std::ranges::end(range);
        }

    }

}

using namespace bu::literals;


template <bu::hashable T>
struct std::hash<std::vector<T>> {
    auto operator()(std::vector<T> const& vector) const -> bu::Usize {
        bu::Usize seed = bu::get_unique_seed();

        for (auto& element : vector) {
            seed = bu::hash_combine_with_seed(seed, element);
        }

        return seed;
    }
};

template <bu::hashable Fst, bu::hashable Snd>
struct std::hash<bu::Pair<Fst, Snd>> {
    auto operator()(bu::Pair<Fst, Snd> const& pair) const
        noexcept(
            std::is_nothrow_invocable_v<std::hash<Fst>, Fst const&> &&
            std::is_nothrow_invocable_v<std::hash<Snd>, Snd const&>
        ) -> bu::Usize
    {
        return bu::hash_combine_with_seed(bu::get_unique_seed(), pair.first, pair.second);
    }
};


#define DEFAULTED_EQUALITY(name) \
[[nodiscard]] auto operator==(name const&) const noexcept -> bool = default


#define DECLARE_FORMATTER_FOR_TEMPLATE(...)                             \
struct std::formatter<__VA_ARGS__> : bu::fmt::Formatter_base {          \
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
DECLARE_FORMATTER_FOR_TEMPLATE(std::vector<T>);
template <class F, class S>
DECLARE_FORMATTER_FOR_TEMPLATE(bu::Pair<F, S>);
template <class Range>
DECLARE_FORMATTER_FOR_TEMPLATE(bu::fmt::Range_formatter_closure<Range>);
template <class T>
DECLARE_FORMATTER_FOR_TEMPLATE(bu::fmt::Integer_with_ordinal_indicator_formatter_closure<T>);


template <class T>
struct std::formatter<std::optional<T>> : std::formatter<T> {
    auto format(std::optional<T> const& optional, std::format_context& context) {
        return optional
            ? std::formatter<T>::format(*optional, context)
            : context.out();
    }
};


template <class... Ts>
DEFINE_FORMATTER_FOR(std::variant<Ts...>) {
    return std::visit([&](auto const& alternative) {
        return std::format_to(context.out(), "{}", alternative);
    }, value);
}

template <class T>
DEFINE_FORMATTER_FOR(std::vector<T>) {
    return std::format_to(context.out(), "{}", bu::fmt::delimited_range(value, ", "));
}

template <class F, class S>
DEFINE_FORMATTER_FOR(bu::Pair<F, S>) {
    return std::format_to(context.out(), "({}, {})", value.first, value.second);
}

template <class Range>
DEFINE_FORMATTER_FOR(bu::fmt::Range_formatter_closure<Range>) {
    if (!value.range->empty()) {
        std::format_to(context.out(), "{}", value.range->front());

        for (auto& element : *value.range | std::views::drop(1)) {
            std::format_to(context.out(), "{}{}", value.delimiter, element);
        }
    }
    return context.out();
}

template <class T>
DEFINE_FORMATTER_FOR(bu::fmt::Integer_with_ordinal_indicator_formatter_closure<T>) {
    // https://stackoverflow.com/questions/61786685/how-do-i-print-ordinal-indicators-in-a-c-program-cant-print-numbers-with-st

    static constexpr auto suffixes = std::to_array<std::string_view>({ "th", "st", "nd", "rd" });

    T x = value.integer % 100;

    if (x == 11 || x == 12 || x == 13) {
        x = 0;
    }
    else {
        x %= 10;
        if (x > 3) {
            x = 0;
        }
    }

    return std::format_to(context.out(), "{}{}", value.integer, suffixes[x]);
}


template <>
struct std::formatter<std::monostate> : bu::fmt::Formatter_base {
    auto format(std::monostate, std::format_context& context) {
        return std::format_to(context.out(), "std::monostate");
    }
};