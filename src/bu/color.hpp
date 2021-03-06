#pragma once

#include "bu/utilities.hpp"


namespace bu {

    enum class Color {
        dark_red,
        dark_green,
        dark_yellow,
        dark_blue,
        dark_purple,
        dark_cyan,
        dark_grey,

        red,
        green,
        yellow,
        blue,
        purple,
        cyan,
        grey,

        black,
        white,

        _color_count
    };

    auto  enable_color_formatting() noexcept -> void;
    auto disable_color_formatting() noexcept -> void;

}


auto operator<<(std::ostream&, bu::Color) -> std::ostream&;

DECLARE_FORMATTER_FOR(bu::Color);