#include "bu/utilities.hpp"
#include "lexical_error.hpp"


namespace {

    auto line_of_occurrence(char const* start, char const* stop, char const* current) -> std::string_view {
        constexpr auto is_space = [](char const c) noexcept -> bool {
            return c == ' ' || c == '\t';
        };

        char const* begin = current;
        for (; begin != start && *begin != '\n'; --begin);
        for (; begin != stop && is_space(*begin); ++begin);

        char const* end = current;
        for (; end != stop && *end != '\n'; ++end);
        for (; end != start && is_space(*end); --end);

        return { begin, end };
    }

}


lexer::Lexical_error::Lexical_error(
    char const*      start,
    char const*      stop,
    std::string_view view,
    std::string_view filename,
    Position         position,
    std::string_view message,
    std::optional<std::string_view> help
)
    : runtime_error
{ [=] {
    assert(std::ranges::count(view, '\n') == 0); // Lexical errors shouldn't span multiple lines

    std::string string;
    string.reserve(256);
    auto out = std::back_inserter(string);

    std::format_to(
        out,
        "Lexical error in {}, on line {}, column {}:",
        filename,
        position.line,
        position.column
    );

    auto const line = line_of_occurrence(start, stop, view.data());

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

    std::format_to(out, "\nMessage: {}\n", message);
    if (help) {
        std::format_to(out, "\nHelpful note: {}\n", *help);
    }

    return string;
}() } {}