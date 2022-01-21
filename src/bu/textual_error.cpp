#include "bu/utilities.hpp"
#include "bu/textual_error.hpp"


namespace {

    constexpr auto whitespace_prefix_length(std::string_view string) noexcept -> bu::Usize {
        bu::Usize prefix = 0;
        for (char const character : string) {
            if (character != ' ') {
                break;
            }
            ++prefix;
        }
        return prefix;
    }

    static_assert(whitespace_prefix_length("test") == 0);
    static_assert(whitespace_prefix_length("  test") == 2);


    auto remove_whitespace_prefix(std::vector<std::string_view>& lines) -> void {
        auto const shortest_prefix_length = std::ranges::min(lines | std::views::transform(whitespace_prefix_length));

        for (auto& line : lines) {
            line.remove_prefix(shortest_prefix_length);
        }
    }


    auto lines_of_occurrence(std::string_view file, std::string_view view) -> std::vector<std::string_view> {
        auto const file_start = file.data();
        auto const view_start = view.data();
        std::vector<std::string_view> lines;

        char const* line_start = view_start;
        for (; line_start != file_start && *line_start != '\n'; --line_start);

        for (char const* pointer = line_start; ; ++pointer) {
            if (*pointer == '\n') {
                lines.push_back({ line_start, pointer });
                line_start = pointer;
            }
            else if (*pointer == '\0') {
                lines.push_back({ line_start, pointer });
                break;
            }
        }

        remove_whitespace_prefix(lines);
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

    std::ignore = type; // for now

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

    auto const lines = lines_of_occurrence(file, view);
    assert(!lines.empty());

    auto const digits      = digit_count(find_position(file, lines.back().data()).line);
    auto       line_number = position.line;

    for (auto line : lines) {
        std::format_to(
            out,
            "\n {:<{}} | {}",
            line_number++,
            digits,
            line
        );
    }

    if (lines.size() == 1) {
        std::format_to(
            out,
            "\n    {:>{}} here",
            std::string(std::max(view.size(), 1_uz), '^'),
            bu::unsigned_distance(lines.front().data(), view.data()) + view.size() + digits
        );
    }

    std::format_to(out, "\n\nMessage: {}\n", message);
    if (help) {
        std::format_to(out, "\nHelpful note: {}\n", *help);
    }

    return string;
}() } {}