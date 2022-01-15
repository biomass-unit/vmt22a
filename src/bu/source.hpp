#pragma once

#include "utilities.hpp"


namespace bu {

    class [[nodiscard]] Source {
        std::string filename;
        std::string contents;
    public:
        explicit Source(std::string&&);

        [[nodiscard]]
        auto string() const noexcept -> std::string_view;
    };

}