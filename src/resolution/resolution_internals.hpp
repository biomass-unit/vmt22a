#pragma once

#include "bu/utilities.hpp"
#include "hir/hir.hpp"
#include "mir/mir.hpp"


namespace resolution {

    struct Context {
        auto resolve(hir::Expression const&) -> mir::Expression;

        auto resolve() noexcept {
            return [this](auto const& node) {
                return resolve(node);
            };
        }
    };

}