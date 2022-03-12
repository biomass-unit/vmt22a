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

    inline auto integer  () -> Value<bu::Isize       > { return {}; }
    inline auto floating () -> Value<bu::Float       > { return {}; }
    inline auto boolean  () -> Value<bool            > { return {}; }
    inline auto string   () -> Value<std::string_view> { return {}; }


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
            Value<std::string_view>
        >;

        Name                            name;
        std::vector<Variant>            values;
        std::optional<std::string_view> description;
        bool                            defaulted = false;
    };


    struct [[nodiscard]] Named_argument {
        using Variant = std::variant<
            bu::Isize,
            bu::Float,
            bool,
            std::string_view
        >;

        std::string          name; // short-form names are automatically converted to long-form
        std::vector<Variant> values;

        auto as_int   () const { return as<0>(); }
        auto as_float () const { return as<1>(); }
        auto as_bool  () const { return as<2>(); }
        auto as_str   () const { return as<3>(); }

        auto nth_as_int   (bu::Usize const index) const { return nth_as<0>(index); }
        auto nth_as_float (bu::Usize const index) const { return nth_as<1>(index); }
        auto nth_as_bool  (bu::Usize const index) const { return nth_as<2>(index); }
        auto nth_as_str   (bu::Usize const index) const { return nth_as<3>(index); }

    private:

        template <bu::Usize alternative>
        auto as() const {
            assert(values.size() == 1);
            return std::get<alternative>(values.at(0));
            // .at(0) used because calling .front() on an empty vector is UB
        }

        template <bu::Usize alternative>
        auto nth_as(bu::Usize const index) const {
            return std::get<alternative>(values.at(index));
        }
    };

    using Positional_argument = std::string_view;


    struct [[nodiscard]] Options_description {
        std::vector<Parameter>         parameters;
        bu::Flatmap<char, std::string> long_forms;

    private:

        struct Option_adder {
            Options_description* self;

            auto map_short_to_long(Parameter::Name const& name) noexcept -> void {
                if (name.short_form) {
                    self->long_forms.add(bu::copy(*name.short_form), bu::copy(name.long_form));
                }
            }

            auto operator()(Parameter::Name&&               name,
                            std::optional<std::string_view> description = std::nullopt)
                noexcept -> Option_adder
            {
                map_short_to_long(name);
                self->parameters.push_back({
                    .name = std::move(name),
                    .description = description
                });
                return *this;
            }

            template <class T>
            auto operator()(Parameter::Name&&               name,
                            Value<T>&&                      value,
                            std::optional<std::string_view> description = std::nullopt)
                noexcept -> Option_adder
            {
                map_short_to_long(name);
                bool const is_defaulted = value.default_value.has_value();

                self->parameters.push_back({
                    .name        = std::move(name),
                    .values      { std::move(value) },
                    .description = description,
                    .defaulted   = is_defaulted
                });
                return *this;
            }

            auto operator()(Parameter::Name&&                 name,
                            std::vector<Parameter::Variant>&& values,
                            std::optional<std::string_view>   description = std::nullopt)
                noexcept -> Option_adder
            {
                map_short_to_long(name);

                auto const has_default = [](auto const& variant) {
                    return std::visit([](auto& alternative) {
                        return alternative.default_value.has_value();
                    }, variant);
                };

                bool is_defaulted = false;
                if (!values.empty()) {
                    is_defaulted = has_default(values.front());
                    auto rest = values | std::views::drop(1);

                    if (is_defaulted) {
                        assert(std::ranges::all_of(rest, has_default));
                    }
                    else {
                        assert(std::ranges::none_of(rest, has_default));
                    }
                }

                self->parameters.emplace_back(
                    std::move(name),
                    std::move(values),
                    description,
                    is_defaulted
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

        auto find_int   (std::string_view const name) noexcept { return find_arg<bu::Isize       >(name); }
        auto find_float (std::string_view const name) noexcept { return find_arg<bu::Float       >(name); }
        auto find_bool  (std::string_view const name) noexcept { return find_arg<bool            >(name); }
        auto find_str   (std::string_view const name) noexcept { return find_arg<std::string_view>(name); }

    private:

        template <class T>
        auto find_arg(std::string_view const name) noexcept -> T* {
            if (auto* const arg = find(name)) {
                assert(arg->values.size() == 1);
                return std::addressof(std::get<T>(arg->values.front()));
                // std::get used instead of std::get_if because invalid access should throw
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