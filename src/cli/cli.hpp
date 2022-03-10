#pragma once

#include "bu/utilities.hpp"
#include "bu/flatmap.hpp"


namespace cli {

    template <class T>
    struct [[nodiscard]] Value {
        std::optional<T> default_value;
        std::optional<T> minimum_value;
        std::optional<T> maximum_value;

        auto default_to(T&& value) noexcept -> Value& {
            default_value = std::move(value);
            return *this;
        }
        auto min(T&& value) noexcept -> Value& {
            minimum_value = std::move(value);
            return *this;
        }
        auto max(T&& value) noexcept -> Value& {
            maximum_value = std::move(value);
            return *this;
        }
    };


    struct [[nodiscard]] Parameter {
        std::string         long_name;
        std::optional<char> short_name;

        std::variant<
            std::monostate,
            Value<bu::Isize>,
            Value<bu::Float>,
            Value<bool>,
            Value<std::string>
        > value;
    };


    struct [[nodiscard]] Named_argument {
        using Variant = std::variant<
            std::monostate,
            bu::Isize,
            bu::Float,
            bool,
            std::string
        >;

        struct Name {
            std::variant<
                char,       // short form, -
                std::string // long form, --
            > value;
        };

        Variant value;
        Name    name;
    };

    struct [[nodiscard]] Positional_argument {
        Named_argument::Variant value;
    };


    struct [[nodiscard]] Options_description {
        std::vector<Parameter> parameters;

        struct Option_adder {
            decltype(parameters)* parameters;

            auto operator()(/* args here */) -> Option_adder {
                return *this;
            }
        };

        auto add_options() noexcept -> Option_adder {
            return { &parameters };
        }
    };


    struct [[nodiscard]] Options {
        std::vector<Positional_argument> positional_arguments;
        std::vector<Named_argument>      named_arguments;
        std::string_view                 program_name_as_invoked;
    };


    auto parse_command_line(int argc, char const** argv, Options_description const&) -> Options;

}