#pragma once

#include "bu/utilities.hpp"
#include "bu/source.hpp"
#include "bu/color.hpp"


namespace bu::diagnostics {

    constexpr Color note_color    = Color::dark_cyan;
    constexpr Color warning_color = Color::dark_yellow;
    constexpr Color error_color   = Color::red;


    enum class Level { normal, error, suppress };
    enum class Type { recoverable, irrecoverable };


    struct Text_section {
        Source_view      source_view;
        Source const*    source     = nullptr;
        std::string_view note       = "here";
        Color            note_color = error_color;
    };


    class Builder {
    public:
        struct Emit_arguments {
            std::vector<Text_section>       sections;
            std::string_view                message_format;
            std::format_args                message_arguments;
            std::optional<std::string_view> help_note;
        };
        struct Simple_emit_arguments {
            bu::Source_view                 erroneous_view;
            Source const*                   source = nullptr;
            std::string_view                message_format;
            std::format_args                message_arguments;
            Color                           message_color = Color::red;
            std::optional<std::string_view> help_note;

            auto to_regular_args() const -> Emit_arguments;
        };
        struct Configuration {
            Level note_level    = Level::normal;
            Level warning_level = Level::normal;
        };
    private:
        std::string   diagnostic_string;
        Configuration configuration;
    public:
        Builder(Configuration = {}) noexcept;
        Builder(Builder&&) noexcept;

        ~Builder();

        auto emit_note       (Emit_arguments)        -> void;
        auto emit_simple_note(Simple_emit_arguments) -> void;

        auto emit_warning       (Emit_arguments)        -> void;
        auto emit_simple_warning(Simple_emit_arguments) -> void;

        [[noreturn]] auto emit_error       (Emit_arguments,        Type = Type::irrecoverable) -> void;
        [[noreturn]] auto emit_simple_error(Simple_emit_arguments, Type = Type::irrecoverable) -> void;

        auto messages() const noexcept -> std::optional<std::string_view> {
            if (diagnostic_string.empty()) {
                return std::nullopt;
            }
            else {
                return diagnostic_string;
            }
        }
    };


    struct Message_arguments {
        std::string_view                message_format;
        std::format_args                message_arguments;
        std::optional<std::string_view> help_note;

        auto add_source_info(bu::Source const*, bu::Source_view) const
            -> Builder::Simple_emit_arguments;
    };

}