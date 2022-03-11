#pragma once

#include "bu/utilities.hpp"
#include "bu/flatmap.hpp"


namespace cli {

    template <class T>
    struct [[nodiscard]] Value {
        std::optional<T> default_value;
        std::optional<T> minimum_value;
        std::optional<T> maximum_value;

        auto default_to(T&& value) && noexcept -> Value&& {
            default_value = std::move(value);
            return std::move(*this);
        }
        auto min(T&& value) && noexcept -> Value&& {
            minimum_value = std::move(value);
            return std::move(*this);
        }
        auto max(T&& value) && noexcept -> Value&& {
            maximum_value = std::move(value);
            return std::move(*this);
        }
    };

    inline auto integer () -> Value<bu::Isize>   { return {}; }
    inline auto floating() -> Value<bu::Float>   { return {}; }
    inline auto boolean () -> Value<bool>        { return {}; }
    inline auto string  () -> Value<std::string> { return {}; }


    struct [[nodiscard]] Parameter {
        struct Name {
            std::string         long_form;
            std::optional<char> short_form;

            Name(char const* long_name, std::optional<char> short_name = std::nullopt) noexcept
                : long_form  { long_name  }
                , short_form { short_name } {}
        };

        using Variant = std::variant<
            Value<bu::Isize>,
            Value<bu::Float>,
            Value<bool>,
            Value<std::string>
        >;

        Name                       name;
        std::optional<Variant>     value;
        std::optional<std::string> description;
    };


    using Argument_value = std::optional<
        std::variant<
            bu::Isize,
            bu::Float,
            bool,
            std::string
        >
    >;

    struct [[nodiscard]] Named_argument {
        std::string    name; // short-form names are automatically converted to long-form
        Argument_value value;
    };

    using Positional_argument = std::string;


    struct [[nodiscard]] Options_description {
        std::vector<Parameter>                                           parameters;
        bu::Flatmap<char, std::string, bu::Flatmap_strategy::store_keys> long_forms;

    private:

        struct Option_adder {
            Options_description* self;

            auto operator()(Parameter::Name&&            name,
                            std::optional<std::string>&& description = std::nullopt)
                noexcept -> Option_adder
            {
                if (name.short_form) {
                    self->long_forms.add(bu::copy(*name.short_form), bu::copy(name.long_form));
                }
                self->parameters.emplace_back(
                    std::move(name),
                    std::nullopt,
                    std::move(description)
                );
                return *this;
            }

            template <class T>
            auto operator()(Parameter::Name&&            name,
                            Value<T>&&                   value,
                            std::optional<std::string>&& description = std::nullopt)
                noexcept -> Option_adder
            {
                if (name.short_form) {
                    self->long_forms.add(bu::copy(*name.short_form), bu::copy(name.long_form));
                }
                self->parameters.emplace_back(
                    std::move(name),
                    std::move(value),
                    std::move(description)
                );
                return *this;
            }
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

        auto find(std::string_view) noexcept -> Named_argument*;

        template <class T>
        auto find_arg(std::string_view const name) noexcept -> T* {
            if (auto* const arg = find(name)) {
                // std::get used instead of std::get_if because invalid access should throw
                return std::addressof(std::get<T>(arg->value.value()));
            }
            else {
                return nullptr;
            }
        }
    };


    struct Unrecognized_option : std::runtime_error {
        using runtime_error::runtime_error;
        using runtime_error::operator=;
    };


    auto parse_command_line(int argc, char const** argv, Options_description const&) -> Options;

}


DECLARE_FORMATTER_FOR(cli::Options_description);