#pragma once

#include "bu/utilities.hpp"
#include "bu/source.hpp"
#include "bu/color.hpp"


namespace bu {

    constexpr Color text_note_color    = Color::dark_blue;
    constexpr Color text_warning_color = Color::dark_yellow;
    constexpr Color text_error_color   = Color::red;


    struct Highlighted_text_section {
        Source_view      source_view;
        Source*          source     = nullptr;
        std::string_view note       = "here";
        Color            note_color = text_error_color;
    };

    struct Textual_error_arguments {
        std::span<Highlighted_text_section const> sections;
        Source*                                   source;
        std::string_view                          message;
        std::optional<std::string_view>           help_note;
    };


    auto textual_error(Textual_error_arguments) -> std::string;

}