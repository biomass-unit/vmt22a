#pragma once

#include "bu/utilities.hpp"
#include "tst/tst.hpp"


namespace dependency {

    struct Module_interface {
        std::filesystem::path           module_path;
        std::vector<Module_interface>   imports;
        std::vector<tst::Definition>    exports;
        std::filesystem::file_time_type last_write_time;


        auto is_recompilation_necessary() const -> bool;

        auto serialize() const -> std::vector<std::byte>;
        static auto deserialize(std::span<std::byte const>) -> Module_interface;
    };

}