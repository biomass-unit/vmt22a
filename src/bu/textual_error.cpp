#include "bu/utilities.hpp"
#include "bu/textual_error.hpp"


namespace {

    auto line_of_occurrence(std::string_view file, char const* current) -> std::string_view {
        constexpr auto is_space = [](char const c) noexcept -> bool {
            return c == ' ' || c == '\t';
        };

        auto const start = file.data();
        auto const stop = start + file.size();

        char const* begin = current;
        for (; begin != start && *begin != '\n'; --begin);
        for (; begin != stop && is_space(*begin); ++begin);

        char const* end = current;
        for (; end != stop && *end != '\n'; ++end);
        for (; end != start && is_space(*end); --end);

        return { begin, end };
    }

    auto find_position(std::string_view file, char const* current) -> bu::Position {
        assert(file.data() <= current && current <= (file.data() + file.size()));
        bu::Position position { 1, 1 };

        for (char const* start = file.data(); start != current; ++start) {
            if (*start == '\n') {
                ++position.line;
                position.column = 1;
            }
            else {
                ++position.column;
            }
        }

        return position;
    }

}


bu::Textual_error::Textual_error(
    Type             type,
    std::string_view view,
    std::string_view file,
    std::string_view filename,
    std::string_view message,
    std::optional<std::string_view> help
)
    : runtime_error
{ [=] {
    std::string string;
    string.reserve(256);
    auto out = std::back_inserter(string);

    auto const position = find_position(file.data(), view.data());

    std::format_to(
        out,
        "In {}, on line {}, column {}:",
        filename,
        position.line,
        position.column
    );

    switch (type) {
    case Type::lexical_error:
    {
        assert(std::ranges::count(view, '\n') == 0); // Lexical errors shouldn't span multiple lines
        auto const line = line_of_occurrence(file, view.data());

        std::format_to(
            out,
            "\n\n |\n | {}\n | {}\n",
            line,
            std::format(
                "{:>{}} here",
                std::string(view.size(), '^'),
                bu::unsigned_distance(line.data(), view.data()) + view.size()
            )
        );

        break;
    }
    case Type::parse_error:
    {
        bu::unimplemented();
    }
    default:
        bu::unimplemented();
    }

    std::format_to(out, "\nMessage: {}\n", message);
    if (help) {
        std::format_to(out, "\nHelpful note: {}\n", *help);
    }

    return string;
}() } {}