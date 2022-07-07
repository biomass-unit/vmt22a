#pragma once

#include "bu/utilities.hpp"
#include "bu/flatmap.hpp"


namespace language {

    constexpr bu::Usize version = 0;


    struct Configuration_key {
        std::string string;

        Configuration_key(std::string&& string) noexcept
            : string { std::move(string) } {}

        template <class T>
        auto parse() const -> std::optional<T> requires std::is_arithmetic_v<T> {
            if (string.empty()) {
                return std::nullopt;
            }
            T value;
            auto const [ptr, ec] = std::from_chars(&string.front(), &string.back(), value);
            return ec == std::errc {} ? value : std::optional<T> {};
        }
    };

    struct Configuration : bu::Flatmap
                         < std::string
                         , std::optional<Configuration_key>
                         , bu::Flatmap_strategy::store_keys
                         >
    {
        using Flatmap::operator=;

        Configuration() = default;

        template <bu::Usize n>
        Configuration(Flatmap::Pairs::value_type (&&array)[n]) noexcept
            : Flatmap(std::vector(std::make_move_iterator(array), std::make_move_iterator(array + n))) {}

        auto operator[](std::string_view) -> Configuration_key&;

        auto string() const -> std::string;
    };


    auto default_configuration() -> Configuration;

    auto read_configuration() -> Configuration;

}