#pragma once


// This file is intended to be included by every single translation unit in the project


#include "bu/disable_unnecessary_warnings.hpp"

#include <cstddef>
#include <cstdint>

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

}