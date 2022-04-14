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


    auto lines_of_occurrence(std::string_view file, std::string_view view)
        -> std::vector<std::string_view>
    {
        auto const file_start = file.data();
        auto const file_stop  = file_start + file.size();
        auto const view_start = view.data();
        auto const view_stop  = view_start + view.size();

        std::vector<std::string_view> lines;

        char const* line_start = view_start;
        for (; line_start != file_start && *line_start != '\n'; --line_start);

        if (*line_start == '\n') {
            ++line_start;
        }

        for (char const* pointer = line_start; ; ++pointer) {
            if (pointer == view_stop) {
                for (; pointer != file_stop && *pointer != '\n'; ++pointer);
                lines.push_back({ line_start, pointer });
                break;
            }
            else if (*pointer == '\n') {
                lines.push_back({ line_start, pointer });
                line_start = pointer + 1;
            }
        }

        remove_surrounding_whitespace(lines);
        return lines;
    }


    auto format_highlighted_section(
        std::back_insert_iterator<std::string> const  out,
        bu::Highlighted_text_section           const& section,
        std::optional<std::string>             const& location_info
    )
        -> void
    {
        auto const lines       = lines_of_occurrence(section.source->string(), section.source_view.string);
        auto const digit_count = bu::digit_count(section.source_view.line + lines.size());
        auto       line_number = section.source_view.line;

        static constexpr auto line_info_color = bu::Color::dark_cyan;

        if (location_info) {
            std::string const whitespace(digit_count, ' ');
            std::format_to(
                out,
                "{}{} --> {}{}\n",
                line_info_color,
                whitespace,
                *location_info,
                bu::Color::white
            );
        }

        for (auto line : lines) {
            std::format_to(
                out,
                "\n {}{:<{}} |{} {}",
                line_info_color,
                line_number++,
                digit_count,
                bu::Color::white,
                line
            );
        }

        if (lines.size() == 1) {
            auto whitespace_length = section.source_view.string.size()
                                   + digit_count
                                   + bu::unsigned_distance(
                                       lines.front().data(),
                                       section.source_view.string.data()
                                   );

            if (section.source_view.string.empty()) { // only reached if the error occurs at EOI
                ++whitespace_length;
            }

            std::format_to(
                out,
                "\n    {}{:>{}} {}{}",
                section.note_color,
                std::string(std::max(section.source_view.string.size(), 1_uz), '^'),
                whitespace_length,
                section.note,
                bu::Color::white
            );
        }
        else {
            bu::abort("multiline errors not supported yet");
        }
    }

}


auto bu::textual_error(Textual_error_arguments const arguments) -> std::string {
    auto const [sections, source, message, help] = arguments;

    assert(!sections.empty());

    std::string string;
    string.reserve(256);
    auto out = std::back_inserter(string);

    std::format_to(out, "{}\n\n", message);

    bu::Source* current_source = nullptr;

    for (auto& section : sections) {
        assert(section.source);

        std::optional<std::string> location_info;

        if (current_source != section.source) {
            current_source = section.source;

            location_info = std::format(
                "{}:{}:{}",
                bu::dtl::filename_without_path(current_source->name()), // fix
                section.source_view.line,
                section.source_view.column
            );
        }

        format_highlighted_section(out, section, location_info);

        if (&section != &sections.back()) {
            std::format_to(out, "\n");
        }
    }

    if (help) {
        std::format_to(out, "\n\nHelpful note: {}", *help);
    }

    return string;
}