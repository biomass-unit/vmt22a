#pragma once

#include "bu/utilities.hpp"
#include "bu/flatmap.hpp"


namespace cli {

    template <class T>
    struct [[nodiscard]] Value {
        std::optional<T> default_value;
        std::optional<T> minimum_value;
        std::optional<T> maximum_value;

        auto default_to(T&&) && noexcept -> Value&&;
        auto min       (T&&) && noexcept -> Value&&;
        auto max       (T&&) && noexcept -> Value&&;
    };

    namespace types {
        using Int   = bu::Isize;
        using Float = bu::Float;
        using Bool  = bool;
        using Str   = std::string_view;
    }

    inline auto integer  () -> Value<types::Int  > { return {}; }
    inline auto floating () -> Value<types::Float> { return {}; }
    inline auto boolean  () -> Value<types::Bool > { return {}; }
    inline auto string   () -> Value<types::Str  > { return {}; }


    struct [[nodiscard]] Parameter {
        struct Name {
            std::string         long_form;
            std::optional<char> short_form;

            Name(char const* long_name, std::optional<char> short_name = std::nullopt) noexcept;
        };

        using Variant = std::variant<
            decltype(integer ()),
            decltype(floating()),
            decltype(boolean ()),
            decltype(string  ())
        >;

        Name                            name;
        std::vector<Variant>            values;
        std::optional<std::string_view> description;
        bool                            defaulted = false;
    };


    struct [[nodiscard]] Named_argument {
        using Variant = std::variant<
            types::Int,
            types::Float,
            types::Bool,
            types::Str
        >;

        std::string          name; // short-form names are automatically converted to long-form, which is also why the string must be an owning one
        std::vector<Variant> values;
    };

    using Positional_argument = std::string_view;


    struct [[nodiscard]] Options_description {
        std::vector<Parameter>         parameters;
        bu::Flatmap<char, std::string> long_forms;

    private:

        struct Option_adder {
            Options_description* self;

            auto map_short_to_long(Parameter::Name const&) noexcept -> void;

            auto operator()(Parameter::Name&&               name,
                            std::optional<std::string_view> description = std::nullopt)
                noexcept -> Option_adder;

            template <class T>
            auto operator()(Parameter::Name&&               name,
                            Value<T>&&                      value,
                            std::optional<std::string_view> description = std::nullopt)
                noexcept -> Option_adder;

            auto operator()(Parameter::Name&&                 name,
                            std::vector<Parameter::Variant>&& values,
                            std::optional<std::string_view>   description = std::nullopt)
                noexcept -> Option_adder;
        };

    public:

        auto add_options() noexcept -> Option_adder {
            return { this };
        }
    };


    struct [[nodiscard]] Options {
        std::vector<Positional_argument> positional_arguments;
        std::vector<Named_argument>      named_arguments;
        std::string_view                 program_name_as_invoked;

        struct Argument_proxy {
            std::string_view         name;
            Named_argument::Variant* pointer = nullptr;
            bu::Usize                count   = 0;
            bool                     indexed = false;
            bool                     empty   = true;

            operator types::Int  *();
            operator types::Float*();
            operator types::Bool *();
            operator types::Str  *();

            explicit operator bool() noexcept;

            auto operator[](bu::Usize) -> Argument_proxy;
        };

        auto operator[](std::string_view) noexcept -> Argument_proxy;

    };


    struct [[nodiscard]] Unrecognized_option : bu::Exception {
        using Exception::Exception;
        using Exception::operator=;
    };


    auto parse_command_line(int argc, char const* const* argv, Options_description const&) -> Options;

}


DECLARE_FORMATTER_FOR(cli::Options_description);