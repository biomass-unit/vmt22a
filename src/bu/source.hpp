#pragma once

#include "utilities.hpp"


namespace bu {

    class [[nodiscard]] Source {
        std::string filename;
        std::string contents;
    public:
        struct Mock_tag {};

        explicit Source(std::string&&);
        explicit Source(Mock_tag, std::string&&);

        [[nodiscard]]
        auto name() const noexcept -> std::string_view;
        [[nodiscard]]
        auto string() const noexcept -> std::string_view;
    };

}