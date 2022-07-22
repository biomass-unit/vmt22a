#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Equality_constraint_visitor {
        resolution::Context& context;
        mir::Type          & left_type;
        mir::Type          & right_type;

        auto recurse(mir::Type& left, mir::Type& right) -> void {
            return std::visit(
                Equality_constraint_visitor {
                    .context    = context,
                    .left_type  = left,
                    .right_type = right
                },
                left.value,
                right.value
            );
        }

        auto operator()(mir::type::Variable const& left, mir::type::Variable const& right) {
            if (left.kind == mir::type::Variable::Kind::general) {
                left_type.value = right;
            }
            else {
                right_type.value = left;
            }
        }

        auto operator()(mir::type::Variable const&, auto const& right) {
            left_type.value = right;
        }

        auto operator()(auto const& left, mir::type::Variable const&) {
            right_type.value = left;
        }

        auto operator()(mir::type::Array const& left, mir::type::Array const& right) {
            recurse(left.element_type, right.element_type);
        }

        auto operator()(auto const&, auto const&) -> void {
            throw bu::exception("Could not unify {} = {}", left_type, right_type);
        }
    };

}


auto resolution::Context::unify(Constraint_set& constraints) -> void {
    for (constraint::Equality& equality_constraint : constraints.equality_constraints) {
        auto& [left, right] = equality_constraint;
        Equality_constraint_visitor { *this, *left, *right }.recurse(*left, *right);
    }
    if (!constraints.instance_constraints.empty()) {
        bu::todo();
    }
}