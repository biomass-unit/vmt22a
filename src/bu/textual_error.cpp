#include "bu/utilities.hpp"
#include "bu/textual_error.hpp"


namespace {

    auto remove_surrounding_whitespace(std::vector<std::string_view>& lines) -> void {
        constexpr auto prefix_length = [](std::string_view const string) {
            return string.find_first_not_of(' ');
        };
        static_assert(prefix_length("test") == 0);
        static_assert(prefix_length("  test") == 2);

        constexpr auto suffix_length = [](std::string_view const string) {
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
        auto const digit_count = bu::digit_count(section.source_view.stop_position.line);
        auto       line_number = section.source_view.start_position.line;

        assert(!lines.empty());

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

        bu::Usize const longest_line_length =
            std::ranges::max(lines | std::views::transform(&std::string_view::size));

        for (auto& line : lines) {
            std::format_to(
                out,
                "\n {}{:<{}} |{} ",
                line_info_color,
                line_number++,
                digit_count,
                bu::Color::white,
                line
            );

            if (lines.size() > 1) {
                if (&line == &lines.front()) {
                    bu::Usize const source_view_offset =
                        bu::unsigned_distance(line.data(), section.source_view.string.data());

                    std::format_to(
                        out,
                        "{}{}{}{}",
                        bu::Color::dark_grey,
                        line.substr(0, source_view_offset),
                        bu::Color::white,
                        line.substr(source_view_offset)
                    );
                }
                else if (&line == &lines.back()) {
                    bu::Usize const source_view_offset =
                        bu::unsigned_distance(
                            line.data(),
                            section.source_view.string.data() + section.source_view.string.size()
                        );

                    std::format_to(
                        out,
                        "{}{}{}{}",
                        line.substr(0, source_view_offset),
                        bu::Color::dark_grey,
                        line.substr(source_view_offset),
                        bu::Color::white
                    );
                }
                else {
                    std::format_to(out, "{}", line);
                }
            }
            else {
                std::format_to(out, "{}", line);
            }

            if (lines.size() > 1) {
                std::format_to(
                    out,
                    "{} {}<",
                    std::string(longest_line_length - line.size(), ' '),
                    section.note_color
                );

                if (&line == &lines.back()) {
                    std::format_to(out, " {}", section.note);
                }

                std::format_to(out, "{}", bu::Color::white);
            }
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

        if (section.source && current_source != section.source) {
            current_source = section.source;

            location_info = std::format(
                "{}:{}-{}",
                bu::dtl::filename_without_path(current_source->name()), // fix
                section.source_view.start_position,
                section.source_view.stop_position
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


auto bu::simple_textual_error(Simple_textual_error_arguments const arguments)
    -> std::string
{
    Highlighted_text_section const section {
        .source_view = arguments.erroneous_view,
        .source      = arguments.source,
        .note        = "here",
        .note_color  = text_error_color
    };

    return textual_error({
        .sections  { &section, 1 }, // Treat the section as a one-element span
        .source    = arguments.source,
        .message   = arguments.message,
        .help_note = arguments.help_note
    });
}