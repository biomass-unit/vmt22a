#pragma once

#include "bu/utilities.hpp"
#include "bu/flatmap.hpp"


namespace language {

    constexpr bu::Usize version = 0;

    struct Configuration : bu::Flatmap
                         < std::string
                         , std::optional<std::string>
                         , bu::Flatmap_strategy::store_keys
                         >
    {
        using Flatmap::Flatmap;
        using Flatmap::operator=;

        auto string() const -> std::string;
    };

    auto default_configuration() -> Configuration;

    auto read_configuration() -> Configuration;

}