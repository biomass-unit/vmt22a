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


    template <class T>
    consteval auto type_description() noexcept -> char const* {
        if constexpr (std::same_as<T, bu::Isize>)
            return "int";
        else if constexpr (std::same_as<T, bu::Float>)
            return "float";
        else if constexpr (std::same_as<T, bool>)
            return "bool";
        else if constexpr (std::same_as<T, std::string_view>)
            return "str";
        else
            static_assert(bu::always_false<T>);
    }

    template <class... Ts>
    constexpr auto type_description(std::variant<Ts...> const& variant) noexcept -> char const* {
        return std::visit([]<class T>(cli::Value<T> const&) { return type_description<T>(); }, variant);
    }


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

        auto error_string(std::string_view const message) const -> std::string {
            // We must recreate the command line as a single string which will
            // emulate the file that would normally be passed to bu::textual_error

            std::string fake_file;
            fake_file.reserve(
                std::transform_reduce(
                    start,
                    stop,
                    bu::unsigned_distance(start, stop), // Account for whitespace delimiters
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

            return bu::textual_error({
                .erroneous_view = erroneous_view,
                .file_view      = fake_file,
                .file_name      = "the command line",
                .message        = message,
            });
        }

        auto error(std::string_view const message) const -> std::runtime_error {
            return std::runtime_error { error_string(message) };
        }

        auto expected(std::string_view const expectation) const -> std::runtime_error {
            return error(std::format("Expected {}", expectation));
        }
    };


    template <class T>
    auto extract_value(Parse_context& context) -> std::optional<T> {
        if (context.is_finished()) {
            return std::nullopt;
        }

        auto const view = context.extract();

        if constexpr (std::same_as<T, bu::Isize> || std::same_as<T, bu::Float>) {
            auto const start = view.data();
            auto const stop  = start + view.size();

            T value;
            auto [ptr, ec] = std::from_chars(start, stop, value);

            switch (ec) {
            case std::errc {}:
            {
                if (ptr == stop) {
                    return value;
                }
                else {
                    context.retreat();
                    throw context.error(
                        std::format(
                            "Unexpected suffix: '{}'",
                            std::string_view { ptr, stop }
                        )
                    );
                }
            }
            case std::errc::result_out_of_range:
            {
                context.retreat();
                throw context.error(
                    std::format(
                        "The given value is too large to be represented by a {}-bit value",
                        sizeof(T) * CHAR_BIT
                    )
                );
            }
            case std::errc::invalid_argument:
            {
                context.retreat();
                return std::nullopt;
            }
            default:
                bu::unimplemented();
            }
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

        else if constexpr (std::same_as<T, std::string_view>) {
            return view;
        }

        else {
            static_assert(bu::always_false<T>);
        }
    }

    auto extract_arguments(Parse_context& context, cli::Parameter const& parameter)
        -> std::vector<cli::Named_argument::Variant>
    {
        std::vector<cli::Named_argument::Variant> arguments;
        arguments.reserve(parameter.values.size());

        for (auto& value : parameter.values) {
            std::visit([&]<class T>(cli::Value<T> const& value) {
                auto argument = extract_value<T>(context);

                if (!argument) {
                    throw context.error(std::format("Expected an argument [{}]", type_description<T>()));
                }

                if (value.minimum_value) {
                    if (*argument < *value.minimum_value) {
                        context.retreat();
                        throw context.error(
                            std::format(
                                "The minimum allowed value is {}",
                                *value.minimum_value
                            )
                        );
                    }
                }
                if (value.maximum_value) {
                    if (*argument > *value.maximum_value) {
                        context.retreat();
                        throw context.error(
                            std::format(
                                "The maximum allowed value is {}",
                                *value.maximum_value
                            )
                        );
                    }
                }

                arguments.push_back(std::move(*argument));
            }, value);
        }

        return arguments;
    }

}


auto cli::parse_command_line(int argc, char const** argv, Options_description const& description)
    -> Options
{
    std::vector<std::string_view> command_line(argv + 1, argv + argc);
    Options options { .program_name_as_invoked = *argv };

    Parse_context context { command_line };

    while (!context.is_finished()) {
        auto name = [&]() -> std::optional<std::string> {
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
                switch (view.size()) {
                case 0:
                    throw context.expected("a single-character flag name");
                case 1:
                    if (auto long_form = description.long_forms.find(view.front())) {
                        return bu::copy(*long_form);
                    }
                    else {
                        context.retreat();
                        throw Unrecognized_option { context.error_string("Unrecognized option") };
                    }
                default:
                    context.retreat();
                    throw context.expected("a single-character flag name; use '--' instead of '-' if this was intended");
                }
            }
            else {
                context.retreat();
                return std::nullopt;
            }
        }();

        if (name) {
            auto it = std::ranges::find(
                description.parameters,
                name,
                bu::compose(&Parameter::Name::long_form, &Parameter::name)
            );

            if (it != description.parameters.end()) {
                options.named_arguments.emplace_back(
                    std::move(*name),
                    extract_arguments(context, *it)
                );
            }
            else {
                context.retreat();
                throw Unrecognized_option { context.error_string("Unrecognized option") };
            }
        }
        else {
            options.positional_arguments.push_back(context.extract());
        }
    }

    // Apply default arguments
    for (auto& parameter : description.parameters) {
        if (parameter.defaulted && !options.find(parameter.name.long_form)) {
            std::vector<Named_argument::Variant> arguments;

            for (auto& value : parameter.values) {
                std::visit([&]<class T>(Value<T> const& value) {
                    arguments.push_back(value.default_value.value());
                }, value);
            }

            options.named_arguments.emplace_back(
                parameter.name.long_form,
                std::move(arguments)
            );
        }
    }

    return options;
}


auto cli::Options::find(std::string_view const name) noexcept -> Named_argument* {
    auto it = std::ranges::find(
        named_arguments,
        name,
        &Named_argument::name
    );

    return it != named_arguments.end()
        ? std::to_address(it)
        : nullptr;
}


DEFINE_FORMATTER_FOR(cli::Options_description) {
    std::vector<bu::Pair<std::string, std::optional<std::string_view>>> lines;
    lines.reserve(value.parameters.size());
    bu::Usize max_length = 0;

    for (auto& [name, arguments, description, _] : value.parameters) {
        std::string line;
        auto out = std::back_inserter(line);

        std::format_to(
            out,
            "--{}{}",
            name.long_form,
            bu::format(name.short_form ? ", -{}" : "", name.short_form)
        );

        for (auto& argument : arguments) {
            std::format_to(out, " [{}]", type_description(argument));
        }

        max_length = std::max(max_length, line.size());
        lines.emplace_back(std::move(line), description);
    }

    for (auto& [names, description] : lines) {
        std::format_to(
            context.out(),
            "\t{:{}}{}\n",
            names,
            max_length,
            bu::format(description ? " : {}" : "", description)
        );
    }

    return context.out();
}