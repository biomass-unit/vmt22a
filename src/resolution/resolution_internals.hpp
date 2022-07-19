#pragma once

#include "bu/utilities.hpp"
#include "bu/safe_integer.hpp"
#include "hir/hir.hpp"
#include "mir/mir.hpp"


namespace resolution {

    struct Namespace {

    };


    

    class Context {
        bu::Safe_usize current_type_variable_tag;
    public:
        mir::Node_context& node_context;

        explicit Context(mir::Node_context& node_context) noexcept
            : node_context { node_context } {}

        auto fresh_type_variable(mir::type::Variable::Kind = mir::type::Variable::Kind::general) -> mir::Type;

        auto resolve(hir::Expression const&) -> mir::Expression;

        auto resolve() noexcept {
            return [this](auto const& node) {
                return resolve(node);
            };
        }
    };

}