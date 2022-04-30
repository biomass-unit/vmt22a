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
    consteval auto type_description() noexcept -> std::string_view {
        if constexpr (std::same_as<T, cli::types::Int>)
            return "int";
        else if constexpr (std::same_as<T, cli::types::Float>)
            return "float";
        else if constexpr (std::same_as<T, cli::types::Bool>)
            return "bool";
        else if constexpr (std::same_as<T, cli::types::Str>)
            return "str";
        else
            static_assert(bu::always_false<T>);
    }

    template <class... Ts>
    constexpr auto type_description(std::variant<Ts...> const& variant) noexcept -> std::string_view {
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
                std::max(
                    std::transform_reduce(
                        start,
                        stop,
                        bu::unsigned_distance(start, stop), // Account for whitespace delimiters
                        std::plus {},
                        std::mem_fn(&std::string_view::size)
                    ),
                    sizeof(std::string) // Disable SSO
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

            bu::Source fake_source {
                bu::Source::Mock_tag { .filename = "command line" },
                std::move(fake_file)
            };

            return bu::simple_textual_error({
                .erroneous_view = bu::Source_view {
                    erroneous_view,
                    bu::Source_position {},
                    bu::Source_position { 1, 1 + bu::unsigned_distance(fake_source.string().data(), erroneous_view.data()) }
                },
                .source  = &fake_source,
                .message = message
            });
        }

        auto error(std::string_view const message) const -> bu::Exception {
            return bu::Exception { error_string(message) };
        }

        auto expected(std::string_view const expectation) const -> bu::Exception {
            return error(std::format("Expected {}", expectation));
        }
    };


    template <class T>
    auto extract_value(Parse_context& context) -> std::optional<T> {
        if (context.is_finished()) {
            return std::nullopt;
        }

        auto const view = context.extract();

        if constexpr (bu::one_of<T, cli::types::Int, cli::types::Float>) {
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

        else if constexpr (std::same_as<T, cli::types::Bool>) {
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

        else if constexpr (std::same_as<T, cli::types::Str>) {
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


template <class T>
auto cli::Value<T>::default_to(T&& value) && noexcept -> Value&& {
    default_value = std::move(value);
    return std::move(*this);
}

template <class T>
auto cli::Value<T>::min(T&& value) && noexcept -> Value&& {
    minimum_value = std::move(value);
    return std::move(*this);
}

template <class T>
auto cli::Value<T>::max(T&& value) && noexcept -> Value&& {
    maximum_value = std::move(value);
    return std::move(*this);
}

template struct cli::Value<cli::types::Int  >;
template struct cli::Value<cli::types::Float>;
template struct cli::Value<cli::types::Bool >;
template struct cli::Value<cli::types::Str  >;


cli::Parameter::Name::Name(char const* long_name, std::optional<char> short_name) noexcept
    : long_form  { long_name  }
    , short_form { short_name } {}


namespace {

    template <bu::Usize n>
    auto arg_as(auto const& values) {
        assert(values.size() == 1);
        return std::get<n>(values.at(0));
        // .at(0) used because calling .front() on an empty vector is UB
    }

    template <bu::Usize n>
    auto nth_arg_as(auto const& values, bu::Usize const index) {
        return std::get<n>(values.at(index));
    }

}

auto cli::Named_argument::as_int   () const -> types::Int   { return arg_as<0>(values); }
auto cli::Named_argument::as_float () const -> types::Float { return arg_as<1>(values); }
auto cli::Named_argument::as_bool  () const -> types::Bool  { return arg_as<2>(values); }
auto cli::Named_argument::as_str   () const -> types::Str   { return arg_as<3>(values); }

auto cli::Named_argument::nth_as_int   (bu::Usize const index) const -> types::Int   { return nth_arg_as<0>(values, index); }
auto cli::Named_argument::nth_as_float (bu::Usize const index) const -> types::Float { return nth_arg_as<1>(values, index); }
auto cli::Named_argument::nth_as_bool  (bu::Usize const index) const -> types::Bool  { return nth_arg_as<2>(values, index); }
auto cli::Named_argument::nth_as_str   (bu::Usize const index) const -> types::Str   { return nth_arg_as<3>(values, index); }


auto cli::Options_description::Option_adder::map_short_to_long(Parameter::Name const& name)
    noexcept -> void
{
    if (name.short_form) {
        self->long_forms.add(bu::copy(*name.short_form), bu::copy(name.long_form));
    }
}


auto cli::Options_description::Option_adder::operator()(Parameter::Name&&               name,
                                                        std::optional<std::string_view> description)
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
auto cli::Options_description::Option_adder::operator()(Parameter::Name&&               name,
                                                        Value<T>&&                      value,
                                                        std::optional<std::string_view> description)
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

auto cli::Options_description::Option_adder::operator()(Parameter::Name&&                 name,
                                                        std::vector<Parameter::Variant>&& values,
                                                        std::optional<std::string_view>   description)
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

template auto cli::Options_description::Option_adder::operator()(Parameter::Name&&, Value<types::Int  >&&, std::optional<std::string_view>) -> Option_adder;
template auto cli::Options_description::Option_adder::operator()(Parameter::Name&&, Value<types::Float>&&, std::optional<std::string_view>) -> Option_adder;
template auto cli::Options_description::Option_adder::operator()(Parameter::Name&&, Value<types::Bool >&&, std::optional<std::string_view>) -> Option_adder;
template auto cli::Options_description::Option_adder::operator()(Parameter::Name&&, Value<types::Str  >&&, std::optional<std::string_view>) -> Option_adder;


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


namespace {

    template <class T>
    auto find_arg(cli::Options* options, std::string_view const name) noexcept -> T* {
        if (auto* const arg = options->find(name)) {
            assert(arg->values.size() == 1);
            return std::addressof(std::get<T>(arg->values.front()));
            // std::get used instead of std::get_if because invalid access should throw
        }
        else {
            return nullptr;
        }
    }

}

auto cli::Options::find_int(std::string_view name) noexcept -> types::Int* {
    return find_arg<types::Int>(this, name);
}
auto cli::Options::find_float(std::string_view name) noexcept -> types::Float* {
    return find_arg<types::Float>(this, name);
}
auto cli::Options::find_bool(std::string_view name) noexcept -> types::Bool* {
    return find_arg<types::Bool>(this, name);
}
auto cli::Options::find_str(std::string_view name) noexcept -> types::Str* {
    return find_arg<types::Str>(this, name);
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
            name.short_form ? std::format(", -{}", *name.short_form) : ""
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
            description ? std::format(" : {}", *description) : ""
        );
    }

    return context.out();
}