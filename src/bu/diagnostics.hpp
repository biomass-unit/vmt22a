#pragma once

#include "bu/utilities.hpp"
#include "bu/source.hpp"
#include "bu/color.hpp"


namespace bu::diagnostics {

    constexpr Color note_color    = Color::cyan;
    constexpr Color warning_color = Color::dark_yellow;
    constexpr Color error_color   = Color::red;


    enum class Level { normal, error, suppress };
    enum class Type { recoverable, irrecoverable };


    struct Text_section {
        Source_view          source_view;
        Source const&        source;
        std::string_view     note = "here";
        std::optional<Color> note_color;

        // Has to be manually defined due to reference member
        auto operator=(Text_section const&) noexcept -> Text_section&;
    };


    class Builder {
    public:
        struct Emit_arguments {
            std::vector<Text_section>       sections;
            std::string_view                message;
            std::format_args                message_arguments;
            std::optional<std::string_view> help_note;
            std::format_args                help_note_arguments;
        };
        struct Simple_emit_arguments {
            bu::Source_view                 erroneous_view;
            Source const&                   source;
            std::string_view                message;
            std::format_args                message_arguments;
            std::optional<std::string_view> help_note;
            std::format_args                help_note_arguments;
        };
        struct Configuration {
            Level note_level    = Level::normal;
            Level warning_level = Level::normal;
        };
    private:
        std::string   diagnostic_string;
        Configuration configuration;
        bool          has_emitted_error;
    public:
        Builder(Configuration = {}) noexcept;
        Builder(Builder&&) noexcept;

        ~Builder();

        auto emit_note       (Emit_arguments)        -> void;
        auto emit_simple_note(Simple_emit_arguments) -> void;

        auto emit_warning       (Emit_arguments)        -> void;
        auto emit_simple_warning(Simple_emit_arguments) -> void;

        [[noreturn]] auto emit_error       (Emit_arguments)        -> void;
        [[noreturn]] auto emit_simple_error(Simple_emit_arguments) -> void;

        auto emit_error       (Emit_arguments,        Type) -> void;
        auto emit_simple_error(Simple_emit_arguments, Type) -> void;

        [[nodiscard]] auto string() && noexcept -> std::string;

        [[nodiscard]] auto error() const noexcept -> bool;

        [[nodiscard]] auto note_level()    const noexcept -> Level;
        [[nodiscard]] auto warning_level() const noexcept -> Level;
    };


    // Thrown when an irrecoverable diagnostic error is emitted
    struct Error : Exception {
        using Exception::Exception;
        using Exception::operator=;
    };


    struct Message_arguments {
        std::string_view                message;
        std::format_args                message_arguments;
        std::optional<std::string_view> help_note;
        std::format_args                help_note_arguments;

        auto add_source_info(bu::Source const&, bu::Source_view) const
            -> Builder::Simple_emit_arguments;
    };

}