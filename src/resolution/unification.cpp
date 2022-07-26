#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Equality_constraint_visitor {
        resolution::Context&             context;
        resolution::constraint::Equality this_constraint;
        bu::Wrapper<mir::Type>           outer_left_type;
        bu::Wrapper<mir::Type>           outer_right_type;


        auto recurse(bu::Wrapper<mir::Type> const left, bu::Wrapper<mir::Type> const right) -> void {
            std::visit(
                Equality_constraint_visitor {
                    .context = context,
                    .this_constraint {
                        .left        = left,
                        .right       = right,
                        .constrainer = this_constraint.constrainer,
                        .constrained = this_constraint.constrained
                    },
                    .outer_left_type  = outer_left_type,
                    .outer_right_type = outer_right_type
                },
                left->value,
                right->value
            );
        }


        auto operator()(mir::type::Integer const& left, mir::type::Integer const& right) -> void {
            if (left != right) {
                bu::todo();
            }
        }

        template <bu::one_of<mir::type::Floating, mir::type::Character, mir::type::Boolean, mir::type::String> T>
        auto operator()(T const&, T const&) -> void {
            // Do nothing
        }

        auto operator()(mir::type::General_variable const&, mir::type::General_variable const& right) -> void {
            this_constraint.left->value = right;
        }
        auto operator()(mir::type::Integral_variable const&, mir::type::Integral_variable const& right) -> void {
            this_constraint.left->value = right;
        }

        auto operator()(mir::type::General_variable const&, mir::type::Integral_variable const& right) -> void {
            this_constraint.left->value = right;
        }
        auto operator()(mir::type::Integral_variable const& left, mir::type::General_variable const&) -> void {
            this_constraint.right->value = left;
        }

        auto operator()(mir::type::Integer const& left, mir::type::Integral_variable const&) -> void {
            this_constraint.right->value = left;
        }
        auto operator()(mir::type::Integral_variable const&, mir::type::Integer const& right) -> void {
            this_constraint.left->value = right;
        }

        auto operator()(mir::type::General_variable const&, auto const& right) -> void {
            this_constraint.left->value = right;
        }
        auto operator()(auto const& left, mir::type::General_variable const&) -> void {
            this_constraint.right->value = left;
        }

        auto operator()(mir::type::Array const& left, mir::type::Array const& right) -> void {
            recurse(left.element_type, right.element_type);
        }

        auto operator()(mir::type::Tuple const& left, mir::type::Tuple const& right) -> void {
            if (left.types.size() == right.types.size()) {
                for (bu::Usize i = 0; i != left.types.size(); ++i) {
                    recurse(left.types[i], right.types[i]);
                }
            }
            else {
                bu::todo();
            }
        }

        auto operator()(mir::type::Function const& left, mir::type::Function const& right) -> void {
            if (left.arguments.size() == right.arguments.size()) {
                for (bu::Usize i = 0; i != left.arguments.size(); ++i) {
                    recurse(left.arguments[i], right.arguments[i]);
                }
                recurse(left.return_type, right.return_type);
            }
            else {
                bu::todo();
            }
        }

        template <bu::one_of<mir::type::Structure, mir::type::Enumeration> T>
        auto operator()(T const& left, T const& right) -> void {
            if (&*left.info != &*right.info) {
                bu::todo();
            }
        }

        auto operator()(auto const&, auto const&) -> void {
            std::string const constrainer_note = std::vformat(
                this_constraint.constrainer.explanatory_note,
                std::make_format_args(
                    outer_left_type,
                    outer_right_type
                )
            );
            std::string const constrained_note = std::vformat(
                this_constraint.constrained.explanatory_note,
                std::make_format_args(
                    outer_left_type,
                    outer_right_type
                )
            );
            context.diagnostics.emit_error({
                .sections = bu::vector_from<bu::diagnostics::Text_section>({
                    {
                        .source_view = this_constraint.constrainer.source_view,
                        .source      = context.source,
                        .note        = constrainer_note
                    },
                    {
                        .source_view = this_constraint.constrained.source_view,
                        .source      = context.source,
                        .note        = constrained_note
                    },
                }),
                .message           = "Could not unify {} = {}",
                .message_arguments = std::make_format_args(this_constraint.left, this_constraint.right)
            });
        }
    };

}


auto resolution::Context::unify(Constraint_set& constraints) -> void {
    for (constraint::Equality const& constraint : constraints.equality_constraints) {
        Equality_constraint_visitor {
            .context          = *this,
            .this_constraint  = constraint,
            .outer_left_type  = constraint.left,
            .outer_right_type = constraint.right
        }.recurse(constraint.left, constraint.right);
    }
    if (!constraints.instance_constraints.empty()) {
        bu::todo();
    }
}