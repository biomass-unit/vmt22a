#pragma once

#include "bu/utilities.hpp"


namespace bu {

    struct [[nodiscard]] Position {
        bu::Usize line;
        bu::Usize column;
    };

    struct Textual_error : std::runtime_error {
        enum class Type { lexical_error, parse_error };

        // Packaging the arguments in a struct allows named arguments via designated initializers
        struct Arguments {
            Type                            error_type;
            std::string_view                erroneous_view;
            std::string_view                file_view;
            std::string_view                file_name;
            std::string_view                message;
            std::optional<std::string_view> help_note;
        };

        explicit Textual_error(Arguments);
    };

}