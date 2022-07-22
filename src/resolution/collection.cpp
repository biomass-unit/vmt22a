#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Constraint_collecting_visitor {
        resolution::Context               & context;
        std::queue<resolution::Constraint>& constraints;
        hir::Expression              const& this_expression;

        auto equality_constraint(bu::Wrapper<mir::Type> const left, bu::Wrapper<mir::Type> const right) -> void {
            constraints.emplace(resolution::constraint::Equality { .left = left, .right = right });
        }

        auto recurse(hir::Expression& expression) -> void {
            std::visit(
                Constraint_collecting_visitor {
                    context,
                    constraints,
                    this_expression
                },
                expression.value
            );
        }


        auto operator()(hir::expression::Array_literal& array) -> void {
            for (hir::Expression& element : array.elements) {
                equality_constraint(
                    bu::get<mir::type::Array>(this_expression.type->value).element_type,
                    element.type
                );
            }
        }

        auto operator()(hir::expression::Type_cast& cast) -> void {
            if (cast.kind == ast::expression::Type_cast::Kind::ascription) {
                equality_constraint(
                    cast.expression->type,
                    context.resolve(*cast.target)
                );
            }
            else {
                bu::todo();
            }
        }

        template <class T>
        auto operator()(T&) -> void {
            bu::abort(typeid(T).name());
        }
    };

}


auto resolution::Context::collect_constraints(hir::Expression& expression)
    -> std::queue<Constraint>
{
    std::queue<Constraint> constraints;
    Constraint_collecting_visitor { *this, constraints, expression }.recurse(expression);
    return constraints;
}