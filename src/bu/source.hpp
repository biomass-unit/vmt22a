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


    struct Source_position {
        Usize line   = 1;
        Usize column = 1;

        auto increment_with(char) noexcept -> void;

        DEFAULTED_EQUALITY(Source_position);
        auto operator<=>(Source_position const&) const noexcept = default;
    };


    struct Source_view {
        std::string_view string;
        Source_position  start_position;
        Source_position  stop_position;

        explicit constexpr Source_view(
            std::string_view const string,
            Source_position  const start,
            Source_position  const stop
        ) noexcept
            : string         { string }
            , start_position { start  }
            , stop_position  { stop   }
        {
            assert(start_position <= stop_position);
        }

        constexpr auto operator==(Source_view const&) const noexcept -> bool {
            // A bit questionable, but necessary for operator== to
            // be able to be defaulted for types with source views
            return true;
        }

        auto operator+(Source_view const&) const noexcept -> Source_view;
    };

}


DECLARE_FORMATTER_FOR(bu::Source_position);