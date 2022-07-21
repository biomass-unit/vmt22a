#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Constraint_collecting_visitor {
        resolution::Context               & context;
        std::queue<resolution::Constraint>& constraints;
        hir::Expression              const& this_expression;

        auto recurse(hir::Expression const& expression) -> void {
            std::visit(
                Constraint_collecting_visitor {
                    context,
                    constraints,
                    this_expression
                },
                expression.value
            );
        }


        auto operator()(hir::expression::Array_literal const& array) -> void {
            for (hir::Expression const& element : array.elements) {
                constraints.emplace(
                    resolution::constraint::Vertical_relationship {
                        .supertype = std::get<mir::type::Array>(this_expression.type->value).element_type,
                        .subtype   = element.type
                    }
                );
            }
        }

        auto operator()(hir::expression::Type_cast const& cast) -> void {
            if (cast.kind == ast::expression::Type_cast::Kind::ascription) {
                constraints.emplace(
                    resolution::constraint::Equality {
                        cast.expression->type,
                        context.resolve(cast.target)
                    }
                );
            }
            else {
                bu::todo();
            }
        }

        template <class T>
        auto operator()(T const&) -> void {
            bu::abort(typeid(T).name());
        }
    };

}


auto resolution::Context::collect_constraints(hir::Expression const& expression)
    -> std::queue<Constraint>
{
    std::queue<Constraint> constraints;
    Constraint_collecting_visitor { *this, constraints, expression }.recurse(expression);
    return constraints;
}