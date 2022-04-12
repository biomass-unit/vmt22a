#pragma once

#include "bu/utilities.hpp"
#include "bu/source.hpp"
#include "bu/color.hpp"


namespace bu {

    // Packaging the arguments in a struct allows named arguments via designated initializers
    struct Textual_error_arguments {
        Source_view                     erroneous_view;
        std::string_view                file_view;
        std::string_view                file_name;
        std::string_view                message;
        std::optional<std::string_view> help_note;
    };

    auto textual_error(Textual_error_arguments) -> std::string;

}