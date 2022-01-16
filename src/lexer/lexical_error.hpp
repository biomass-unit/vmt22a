#pragma once

#include "bu/utilities.hpp"


namespace lexer {

    struct [[nodiscard]] Position {
        bu::Usize line;
        bu::Usize column;
    };

    struct Lexical_error : std::runtime_error {
        explicit Lexical_error(
            char const*      start,
            char const*      stop,
            std::string_view view,
            std::string_view filename,
            Position         position,
            std::string_view message,
            std::optional<std::string_view> help
        );
    };

}