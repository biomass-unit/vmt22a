#pragma once

#include "utilities.hpp"


namespace bu {

    class [[nodiscard]] Source {
        std::string filename;
        std::string contents;
    public:
        struct Mock_tag { std::string_view filename; };

        explicit Source(std::string&&);
        explicit Source(Mock_tag, std::string&&);

        [[nodiscard]]
        auto name() const noexcept -> std::string_view;
        [[nodiscard]]
        auto string() const noexcept -> std::string_view;
    };

    struct Source_view {
        std::string_view string;
        Usize            line;
        Usize            column;

        explicit constexpr Source_view(
            std::string_view const string,
            Usize            const line,
            Usize            const column
        ) noexcept
            : string { string }
            , line   { line   }
            , column { column } {}

        constexpr auto operator==(Source_view const&) const noexcept -> bool {
            // A bit questionable, but necessary for operator== to
            // be able to be defaulted for types with source views
            return true;
        }

        constexpr auto operator+(Source_view const& other) const noexcept -> Source_view {
            return Source_view {
                std::string_view {
                    string.data(),
                    other.string.data() + other.string.size()
                },
                line,
                column
            };
        }
    };

}