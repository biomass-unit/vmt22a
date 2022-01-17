#pragma once

#include "bu/utilities.hpp"


namespace bu {

    struct [[nodiscard]] Position {
        bu::Usize line;
        bu::Usize column;
    };

    struct Textual_error : std::runtime_error {
        enum class Type { lexical_error, parse_error };

        explicit Textual_error(
            Type             type,
            std::string_view view,
            std::string_view file,
            std::string_view filename,
            std::string_view message,
            std::optional<std::string_view> help
        );
    };

}