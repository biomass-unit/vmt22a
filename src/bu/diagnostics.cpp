#include "bu/utilities.hpp"
#include "diagnostics.hpp"


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
        char const* const file_start = file.data();
        char const* const file_stop  = file_start + file.size();
        char const* const view_start = view.data();
        char const* const view_stop  = view_start + view.size();

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
        bu::Color                              const  title_color,
        bu::diagnostics::Text_section          const& section,
        std::optional<std::string>             const& location_info
    )
        -> void
    {
        auto const lines       = lines_of_occurrence(section.source.string(), section.source_view.string);
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
                    section.note_color.value_or(title_color)
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
                section.note_color.value_or(title_color),
                std::string(std::max(section.source_view.string.size(), 1_uz), '^'),
                whitespace_length,
                section.note,
                bu::Color::white
            );
        }
    }


    auto do_emit(
        std::string&                                   diagnostic_string,
        bu::diagnostics::Builder::Emit_arguments const arguments,
        std::string_view                         const title,
        bu::Color                                const title_color,
        bu::diagnostics::Type                    const diagnostic_type = bu::diagnostics::Type::recoverable) -> void
    {
        auto& [sections, message_format, message_format_arguments, help_note] = arguments;

        assert(!arguments.sections.empty());
        auto out = std::back_inserter(diagnostic_string);

        if (!diagnostic_string.empty()) { // there are previous diagnostic messages
            std::format_to(out, "\n\n\n");
        }

        std::format_to(out, "{}{}{}: ", title_color, title, bu::Color::white);

        std::vformat_to(out, message_format, message_format_arguments);
        std::format_to(out, "\n\n");

        bu::Source const* current_source = nullptr;

        for (auto& section : sections) {
            bu::always_assert(section.source_view.string.empty()
                           || section.source_view.string.front() != '\0');

            std::optional<std::string> location_info;

            if (current_source != &section.source) {
                current_source = &section.source;

                location_info = std::format(
                    "{}:{}-{}",
                    bu::dtl::filename_without_path(current_source->name()), // fix
                    section.source_view.start_position,
                    section.source_view.stop_position
                );
            }

            format_highlighted_section(out, title_color, section, location_info);

            if (&section != &sections.back()) {
                std::format_to(out, "\n");
            }
        }

        if (help_note) {
            std::format_to(out, "\n\nHelpful note: {}", *help_note);
        }

        if (diagnostic_type == bu::diagnostics::Type::irrecoverable) {
            throw bu::diagnostics::Error { std::move(diagnostic_string) };
        }
    }


    auto to_regular_args(
        bu::diagnostics::Builder::Simple_emit_arguments const arguments,
        bu::Color                                       const note_color
    )
        -> bu::diagnostics::Builder::Emit_arguments
    {
        return {
            .sections {
                bu::diagnostics::Text_section {
                    .source_view = arguments.erroneous_view,
                    .source      = arguments.source,
                    .note        = "here",
                    .note_color  = note_color
                }
            },
            .message_format    = arguments.message_format,
            .message_arguments = arguments.message_arguments,
            .help_note         = arguments.help_note
        };
    }

}


bu::diagnostics::Builder::Builder(Configuration const configuration) noexcept
    : configuration { configuration }
    , has_emitted_error { false } {}

bu::diagnostics::Builder::Builder(Builder&& other) noexcept
    : diagnostic_string { std::move(other.diagnostic_string) }
    , configuration     { other.configuration }
    , has_emitted_error { other.has_emitted_error }
{
    other.diagnostic_string.clear();
    other.has_emitted_error = false;
    // A moved-from std::string is not guaranteed to be empty, so
    // this move constructor has to be written by hand, because the
    // destructor relies on moved-from diagnostic strings being empty.
}

bu::diagnostics::Builder::~Builder() {
    if (!diagnostic_string.empty()) {
        std::cout << diagnostic_string << "\n\n"; // TODO: improve
    }
}

auto bu::diagnostics::Builder::messages() const noexcept -> std::optional<std::string_view> {
    if (diagnostic_string.empty()) {
        return std::nullopt;
    }
    else {
        return diagnostic_string;
    }
}

auto bu::diagnostics::Builder::error() const noexcept -> bool {
    return has_emitted_error;
}

auto bu::diagnostics::Builder::note_level() const noexcept -> Level {
    return configuration.note_level;
}
auto bu::diagnostics::Builder::warning_level() const noexcept -> Level {
    return configuration.warning_level;
}


auto bu::diagnostics::Builder::emit_note(Emit_arguments const arguments) -> void {
    switch (configuration.note_level) {
    case Level::normal:
        return do_emit(diagnostic_string, arguments, "Note", note_color);
    case Level::error:
        return do_emit(diagnostic_string, arguments, "The following note is treated as an error", error_color);
    case Level::suppress:
        return;
    default:
        std::unreachable();
    }
}

auto bu::diagnostics::Builder::emit_warning(Emit_arguments const arguments) -> void {
    switch (configuration.warning_level) {
    case Level::normal:
        return do_emit(diagnostic_string, arguments, "Warning", warning_color);
    case Level::error:
        return do_emit(diagnostic_string, arguments, "The following warning is treated as an error", error_color);
    case Level::suppress:
        return;
    default:
        std::unreachable();
    }
}

auto bu::diagnostics::Builder::emit_error(
    Emit_arguments const arguments,
    Type           const error_type) -> void
{
    has_emitted_error = true;
    do_emit(diagnostic_string, arguments, "Error", error_color, error_type);
}


auto bu::diagnostics::Builder::emit_simple_note(Simple_emit_arguments const arguments) -> void {
    return emit_note(to_regular_args(arguments, note_color));
}

auto bu::diagnostics::Builder::emit_simple_warning(Simple_emit_arguments const arguments) -> void {
    return emit_warning(to_regular_args(arguments, warning_color));
}

auto bu::diagnostics::Builder::emit_simple_error(
    Simple_emit_arguments const arguments,
    Type                  const error_type) -> void
{
    emit_error(to_regular_args(arguments, error_color), error_type);
}


auto bu::diagnostics::Message_arguments::add_source_info(
    bu::Source      const& source,
    bu::Source_view const  erroneous_view) const -> Builder::Simple_emit_arguments
{
    return {
        .erroneous_view    = erroneous_view,
        .source            = source,
        .message_format    = message_format,
        .message_arguments = message_arguments,
        .help_note         = help_note
    };
}