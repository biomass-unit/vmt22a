#include "bu/utilities.hpp"
#include "bu/textual_error.hpp"


namespace {

    auto remove_surrounding_whitespace(std::vector<std::string_view>& lines) -> void {
        constexpr auto prefix_length = [](std::string_view string) {
            return string.find_first_not_of(' ');
        };
        static_assert(prefix_length("test") == 0);
        static_assert(prefix_length("  test") == 2);

        constexpr auto suffix_length = [](std::string_view string) {
            return string.size() - string.find_last_not_of(' ') - 1;
        };
        static_assert(suffix_length("test") == 0);
        static_assert(suffix_length("test  ") == 2);

        auto const shortest_prefix_length = std::ranges::min(lines | std::views::transform(prefix_length));

        for (auto& line : lines) {
            line.remove_prefix(shortest_prefix_length);
            line.remove_suffix(suffix_length(line));
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

        remove_surrounding_whitespace(lines);
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

}


bu::Textual_error::Textual_error(Arguments const arguments)
    : runtime_error
{ [=] {
    auto const [view, file, filename, message, help] = arguments;
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

    auto const lines       = lines_of_occurrence(file, view);
    auto const digit_count = bu::digit_count(find_position(file, lines.back().data()).line);
    auto       line_number = position.line;

    for (auto line : lines) {
        std::format_to(
            out,
            "\n {:<{}} | {}",
            line_number++,
            digit_count,
            line
        );
    }

    if (lines.size() == 1) {
        auto whitespace_length =
            view.size() + digit_count + bu::unsigned_distance(lines.front().data(), view.data());

        if (view.size() == 0) { // only reached if the error occurs at EOI
            ++whitespace_length;
        }

        std::format_to(
            out,
            "\n    {}{:>{}} {}here",
            bu::Color::red,
            std::string(std::max(view.size(), 1_uz), '^'),
            whitespace_length,
            bu::Color::white
        );
    }

    std::format_to(out, "\n\nMessage: {}\n", message);
    if (help) {
        std::format_to(out, "\nHelpful note: {}\n", *help);
    }

    return string;
}() } {}