#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Equality_constraint_visitor {
        resolution::Context& context;
        mir::Type          & left_type;
        mir::Type          & right_type;

        auto recurse(mir::Type& left, mir::Type& right) -> void {
            std::visit(
                Equality_constraint_visitor {
                    .context    = context,
                    .left_type  = left,
                    .right_type = right
                },
                left.value,
                right.value
            );
        }


        auto operator()(mir::type::Integer const left, mir::type::Integer const right) -> void {
            if (left != right) {
                bu::todo();
            }
        }

        template <bu::one_of<mir::type::Floating, mir::type::Character, mir::type::Boolean, mir::type::String> T>
        auto operator()(T const&, T const&) -> void {
            // Do nothing
        }

        auto operator()(mir::type::General_variable const&, mir::type::General_variable const& right) -> void {
            left_type.value = right;
        }
        auto operator()(mir::type::Integral_variable const&, mir::type::Integral_variable const& right) -> void {
            left_type.value = right;
        }

        auto operator()(mir::type::General_variable const&, mir::type::Integral_variable const& right) -> void {
            left_type.value = right;
        }
        auto operator()(mir::type::Integral_variable const& left, mir::type::General_variable const&) -> void {
            right_type.value = left;
        }

        auto operator()(mir::type::Integer const& left, mir::type::Integral_variable const&) -> void {
            right_type.value = left;
        }
        auto operator()(mir::type::Integral_variable const&, mir::type::Integer const& right) -> void {
            left_type.value = right;
        }

        auto operator()(mir::type::General_variable const&, auto const& right) -> void {
            left_type.value = right;
        }
        auto operator()(auto const& left, mir::type::General_variable const&) -> void {
            right_type.value = left;
        }

        auto operator()(mir::type::Array const& left, mir::type::Array const& right) -> void {
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