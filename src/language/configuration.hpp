#pragma once

#include "bu/utilities.hpp"
#include "bu/flatmap.hpp"


namespace language {

    constexpr bu::Usize version = 0;

    using Configuration = bu::Flatmap<std::string, std::string, bu::Flatmap_strategy::store_keys>;

    auto read_configuration() -> Configuration;

}