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

    auto lines_of_occurrence(std::string_view file, std::string_view view) -> std::vector<std::string_view> {
        auto const [file_start, file_stop] = bu::to_pointers(file);
        auto const [view_start, view_stop] = bu::to_pointers(view);
        std::vector<std::string_view> lines;

        char const* line_start = view_start;
        for (; line_start != file_start && *line_start != '\n'; --line_start);

        for (char const* pointer = line_start; pointer != view_stop; ++pointer) {
            if (*pointer == '\n') {
                lines.push_back({ line_start, pointer });
                line_start = pointer;
            }
        }
        if (lines.size() != 1) {
            lines.push_back({ line_start, view_stop });
        }

        constexpr auto whitespace_prefix_length = [](std::string_view string) noexcept -> bu::Usize {
            bu::Usize prefix = 0;
            for (char const character : string) {
                if (character != ' ') {
                    break;
                }
                ++prefix;
            }
            return prefix;
        };

        static_assert(whitespace_prefix_length("test") == 0);
        static_assert(whitespace_prefix_length("  test") == 2);

        auto const shortest_prefix_length = std::ranges::min(lines | std::views::transform(whitespace_prefix_length));

        for (auto& line : lines) {
            line.remove_prefix(shortest_prefix_length);
        }

        return lines;
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


    constexpr auto digit_count(bu::Usize integer) noexcept -> bu::Usize {
        bu::Usize digits = 0;
        do {
            integer /= 10;
            ++digits;
        }
        while (integer);
        return digits;
    }

    static_assert(digit_count(0) == 1);
    static_assert(digit_count(10) == 2);
    static_assert(digit_count(999) == 3);
    static_assert(digit_count(1234) == 4);

}


bu::Textual_error::Textual_error(Arguments const arguments)
    : runtime_error
{ [=] {
    auto const [type, view, file, filename, message, help] = arguments;
    auto const position = find_position(file.data(), view.data());

    std::string string;
    string.reserve(256);
    auto out = std::back_inserter(string);

    std::format_to(
        out,
        "In {}, on line {}, column {}:\n",
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
            "\n |\n | {}\n | {}",
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
        auto const lines = lines_of_occurrence(file, view);
        assert(!lines.empty());
        auto const digits = digit_count(find_position(file, lines.back().data()).line);

        auto line_number = position.line;

        for (auto line : lines) {
            std::format_to(
                out,
                "\n {:<{}} | {}",
                line_number++,
                digits,
                line
            );
        }

        break;
    }
    default:
        bu::unimplemented();
    }

    std::format_to(out, "\n\nMessage: {}\n", message);
    if (help) {
        std::format_to(out, "\nHelpful note: {}\n", *help);
    }

    return string;
}() } {}