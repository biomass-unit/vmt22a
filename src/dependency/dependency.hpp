#pragma once

#include "bu/utilities.hpp"
#include "ast/ast.hpp"
#include "tst/tst.hpp"
//#include "vm/"


namespace dependency {

    struct Module_interface {

        struct Visible_components {
            std::vector<Module_interface> exported_modules;

            auto hash() const -> bu::Usize;
        };

        struct Invisible_components {
            std::vector<Module_interface> imported_modules;

            auto hash() const -> bu::Usize;
        };

        Visible_components   visible;
        Invisible_components invisible;

        std::filesystem::path module_path;

        auto serialize() const -> std::vector<std::byte>;
        static auto deserialize(std::span<std::byte const>) -> Module_interface;

    };

}