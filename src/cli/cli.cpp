#include "bu/utilities.hpp"
#include "bu/textual_error.hpp"
#include "cli.hpp"

#include <charconv>


namespace {

    constexpr auto to_lower(char const c) noexcept -> char {
        return ('A' <= c && c <= 'Z') ? c + 32 : c;
    }

    static_assert(to_lower('A') == 'a');
    static_assert(to_lower('z') == 'z');
    static_assert(to_lower('%') == '%');


    struct Parse_context {
        std::string_view* pointer;
        std::string_view* start;
        std::string_view* stop;

        explicit Parse_context(std::span<std::string_view> const span) noexcept
            : pointer { span.data()         }
            , start   { pointer             }
            , stop    { start + span.size() } {}

        auto is_finished() const noexcept -> bool {
            return pointer == stop;
        }

        auto current() const noexcept -> std::string_view& {
            return *pointer;
        }

        auto extract() noexcept -> std::string_view& {
            return *pointer++;
        }

        auto advance() noexcept -> void {
            ++pointer;
        }

        auto retreat() noexcept -> void {
            --pointer;
        }

        auto error(std::string_view const message) -> bu::Textual_error {
            std::string fake_file;
            fake_file.reserve(
                std::transform_reduce(
                    start,
                    stop,
                    bu::unsigned_distance(start, stop), // account for whitespace delimiters
                    std::plus {},
                    std::mem_fn(&std::string_view::size)
                )
            );

            for (auto view = start; view != stop; ++view) {
                fake_file.append(*view) += ' ';
            }

            // The erroneous view must be a view into the fake_file
            std::string_view erroneous_view;

            if (pointer == stop) {
                auto const end = fake_file.data() + fake_file.size();
                erroneous_view = { end - 1, end };
            }
            else {
                char const* view_begin = fake_file.data();
                for (auto view = start; view != pointer; ++view) {
                    view_begin += view->size() + 1; // +1 for the whitespace delimiter
                }
                erroneous_view = { view_begin, pointer->size() };
            }

            return bu::Textual_error {
                bu::Textual_error::Arguments {
                    .erroneous_view = erroneous_view,
                    .file_view      = fake_file,
                    .file_name      = "the command line",
                    .message        = message,
                }
            };
        }

        auto expected(std::string_view const expectation) -> bu::Textual_error {
            return error(std::format("Expected {}", expectation));
        }
    };


    template <class T>
    auto extract_value(Parse_context& context) -> std::optional<T> {
        if (context.is_finished()) {
            return std::nullopt;
        }

        auto const view = context.extract();

        if constexpr (std::same_as<T, bu::Isize>) {
            bu::unimplemented();
        }

        else if constexpr (std::same_as<T, bu::Float>) {
            bu::unimplemented();
        }

        else if constexpr (std::same_as<T, bool>) {
            std::string input;
            input.reserve(view.size());
            std::ranges::copy(view | std::views::transform(to_lower), std::back_inserter(input));

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
                context.retreat();
                return std::nullopt;
            }
        }

        else if constexpr (std::same_as<T, std::string>) {
            return std::string { view };
        }

        else {
            static_assert(bu::always_false<T>);
        }
    }

    auto extract_argument(Parse_context& context, cli::Parameter const& parameter)
        -> cli::Argument_value
    {
        using R = cli::Argument_value;

        if (!parameter.value) {
            return std::nullopt;
        }

        return std::visit(bu::Overload {
            [&]<class T>(cli::Value<T> const& value) -> R {
                auto argument = extract_value<T>(context);

                if (!argument) {
                    if (value.default_value) {
                        argument = value.default_value;
                    }
                    else {
                        bu::abort("no argument supplied");
                    }
                }

                if (value.minimum_value) {
                    if (*argument < *value.minimum_value) {
                        bu::abort("value too small");
                    }
                }
                if (value.maximum_value) {
                    if (*argument > *value.maximum_value) {
                        bu::abort("value too large");
                    }
                }

                return std::move(*argument);
            }
        }, *parameter.value);
    }

}


auto cli::parse_command_line(int argc, char const** argv, Options_description const& description)
    -> Options
{
    std::vector<std::string_view> command_line(argv + 1, argv + argc);
    Options options { .program_name_as_invoked = *argv };

    Parse_context context { command_line };

    while (!context.is_finished()) {
        auto name = [&]() -> std::optional<Named_argument::Name> {
            auto view = context.extract();

            if (view.starts_with("--")) {
                view.remove_prefix(2);
                if (view.empty()) {
                    throw context.expected("a flag name");
                }
                else {
                    return std::string { view };
                }
            }
            else if (view.starts_with('-')) {
                view.remove_prefix(1);
                if (view.size() != 1) {
                    throw context.expected("a single-character argument name");
                }
                else {
                    return view.front();
                }
            }
            else {
                return std::nullopt;
            }
        }();

        if (name) {
            auto it = std::visit(bu::Overload {
                [&](std::string_view const name) {
                    return std::ranges::find(
                        description.parameters,
                        name,
                        bu::compose(&Parameter::Name::long_form, &Parameter::name)
                    );
                },
                [&](char const name) {
                    return std::ranges::find(
                        description.parameters,
                        std::optional { name },
                        bu::compose(&Parameter::Name::short_form, &Parameter::name)
                    );
                }
            }, *name);

            if (it != description.parameters.end()) {
                options.named_arguments.emplace_back(
                    std::move(*name),
                    extract_argument(context, *it)
                );
            }
            else {
                context.retreat();
                throw context.error("Unrecognized option");
            }
        }
        else {
            bu::unimplemented();
        }
    }

    return options;
}


auto cli::Options::find(std::string_view const) noexcept -> Named_argument* {
    bu::unimplemented();
}

auto cli::Options::find(char const) noexcept -> Positional_argument* {
    bu::unimplemented();
}


DEFINE_FORMATTER_FOR(cli::Options_description) {
    std::vector<bu::Pair<std::string, std::optional<std::string>>> lines;
    lines.reserve(value.parameters.size());
    bu::Usize max_length = 0;

    for (auto& [name, argument, description] : value.parameters) {
        lines.emplace_back(
            std::format(
                "--{}{}{}",
                name.long_form,
                std::format(name.short_form ? ", -{}" : "", name.short_form),
                argument ? " [arg]" : ""
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