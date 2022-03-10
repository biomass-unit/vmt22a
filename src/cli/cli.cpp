#include "bu/utilities.hpp"
#include "cli.hpp"

#include <charconv>


namespace {

    template <class T>
    auto extract_value(std::string_view view) -> std::optional<T> {
        if constexpr (std::same_as<T, bu::Isize>) {
            bu::unimplemented();
        }

        else if constexpr (std::same_as<T, bu::Float>) {
            bu::unimplemented();
        }

        else if constexpr (std::same_as<T, bool>) {
            std::string input;
            input.reserve(view.size());
            for (char const c : view) { input.push_back((char)std::tolower(c)); }

            auto const is_one_of = [&](auto const&... args) {
                return ((input == args) || ...);
            };

            if (is_one_of("true", "yes", "1")) {
                return true;
            }
            else if (is_one_of("false", "no", "0")) {
                return false;
            }
            else {
                return std::nullopt;
            }
        }

        else if constexpr (std::same_as<T, std::string>) {
            bu::unimplemented();
        }

        else {
            static_assert(bu::always_false<T>);
        }
    }

    auto extract_argument(std::string_view*&    start,
                          std::string_view*     stop,
                          cli::Parameter const& parameter)
        -> cli::Named_argument::Variant
    {
        using R = cli::Named_argument::Variant;

        return std::visit(bu::Overload {
            [](std::monostate) -> R {
                return std::monostate {};
            },
            [&]<class T>(cli::Value<T> const& value) -> R {
                if (start == stop) {
                    bu::unimplemented();
                }

                auto argument = extract_value<T>(*start);

                if (!argument) {
                    if (value.default_value) {
                        argument = value.default_value;
                    }
                    else {
                        bu::abort("no argument supplied");
                    }
                }

                return std::move(*argument);
            }
        }, parameter.value);
    }

}


auto cli::parse_command_line(int argc, char const** argv, Options_description const& description)
    -> Options
{
    std::vector<std::string_view> command_line(argv + 1, argv + argc);
    Options options { .program_name_as_invoked = *argv };

    auto       token = command_line.data();
    auto const end   = token + command_line.size();

    auto const expected = [&](std::string_view expectation) {
        return std::runtime_error { std::format("Expected {}", expectation) };
    };

    for (; token != end; ++token) {
        std::optional<std::variant<char, std::string>> name;

        if (!token->empty()) {
            if (token->starts_with("--")) {
                token->remove_prefix(2);
                if (token->empty()) {
                    throw expected("an argument name");
                }
                else {
                    name = std::string { *token };
                }
            }
            else if (token->starts_with('-')) {
                token->remove_prefix(1);
                if (token->size() != 1) {
                    throw expected("a single-character argument name");
                }
                else {
                    name = token->front();
                }
            }
            else {
                bu::unimplemented(); // positional
            }

            auto it = std::visit(bu::Overload {
                [&](std::string_view const name) {
                    return std::ranges::find(
                        description.parameters,
                        name,
                        &Parameter::long_name
                    );
                },
                [&](char const name) {
                    return std::ranges::find(
                        description.parameters,
                        std::optional { name },
                        &Parameter::short_name
                    );
                }
            }, *name);

            if (it != description.parameters.end()) {
                options.named_arguments.emplace_back(
                    extract_argument(++token, end, *it),
                    std::move(*name)
                );
            }
            else {
                bu::abort("undeclared parameter");
            }
        }
    }

    return options;
}


DEFINE_FORMATTER_FOR(cli::Options_description) {
    std::vector<bu::Pair<std::string, std::optional<std::string>>> lines;
    lines.reserve(value.parameters.size());
    bu::Usize max_length = 0;

    for (auto& [long_name, short_name, argument, description] : value.parameters) {
        lines.emplace_back(
            std::format(
                "--{}{}{}",
                long_name,
                std::format(short_name ? ", -{}" : "", short_name),
                std::holds_alternative<std::monostate>(argument) ? "" : " [arg]"
            ),
            description
        );

        max_length = std::max(max_length, lines.back().first.size());
    }

    for (auto& [names, description] : lines) {
        std::format_to(
            context.out(),
            "\t{:{}}{}\n",
            names,
            max_length,
            std::format(description ? " : {}" : "", description)
        );
    }

    return context.out();
}