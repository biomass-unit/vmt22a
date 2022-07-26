#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Expression_resolution_visitor {
        resolution::Context       & context;
        resolution::Scope         & scope;
        resolution::Namespace     & space;
        resolution::Constraint_set& constraint_set;
        hir::Expression           & this_expression;

        auto recurse(hir::Expression& expression, resolution::Scope* const new_scope = nullptr)
            -> mir::Expression
        {
            return std::visit(
                Expression_resolution_visitor {
                    context,
                    new_scope ? *new_scope : scope,
                    space,
                    constraint_set,
                    expression
                },
                expression.value
            );
        }


        template <class T>
        auto operator()(hir::expression::Literal<T>& literal) -> mir::Expression {
            return {
                .value       = literal,
                .type        = this_expression.type,
                .source_view = this_expression.source_view
            };
        }

        auto operator()(hir::expression::Array_literal& array) -> mir::Expression {
            bu::wrapper auto const element_type =
                bu::get<mir::type::Array>(this_expression.type->value).element_type;

            mir::expression::Array_literal mir_array;
            mir_array.elements.reserve(array.elements.size());

            if (!array.elements.empty()) {
                mir_array.elements.push_back(recurse(array.elements.front()));
                bu::wrapper auto const first_element_type = mir_array.elements.back().type;

                hir::Expression* previous_element = &array.elements.front();

                for (hir::Expression& element : array.elements | std::views::drop(1)) {
                    constraint_set.equality_constraints.push_back({
                        .left  = first_element_type,
                        .right = element.type,
                        .constrainer {
                            array.elements.front().source_view + previous_element->source_view,
                            &element == &array.elements[1]
                                ? "The previous element was of type {0}"
                                : "The previous elements were of type {0}"
                        },
                        .constrained {
                            element.source_view,
                            "But this element is of type {1}"
                        }
                    });

                    mir_array.elements.push_back(recurse(element));
                    previous_element = &element;
                }

                element_type->value = first_element_type->value;
            }

            return {
                .value       = std::move(mir_array),
                .type        = this_expression.type,
                .source_view = this_expression.source_view
            };
        }

        auto operator()(hir::expression::Variable& variable) -> mir::Expression {
            if (variable.name.is_unqualified()) {
                if (auto* const binding = scope.find_variable(variable.name.primary_name.identifier)) {
                    this_expression.type = binding->type;
                    binding->has_been_mentioned = true;
                    return {
                        .value       = mir::expression::Local_variable_reference { variable.name.primary_name },
                        .type        = binding->type,
                        .source_view = this_expression.source_view
                    };
                }
            }

            if (auto info = context.find_function(scope, space, variable.name)) {
                context.resolve_function(*info);
                this_expression.type->value = (*info)->function_type->value;

                /*constraint_set.equality_constraints.push_back({
                    .left  = (*info)->function_type,
                    .right = this_expression.type,
                    .constrainer {
                        std::visit([](auto const& function) { return function.name.source_view; }, (*info)->value),
                        ""
                    },
                    .constrained {
                        this_expression.source_view,
                        ""
                    }
                });*/

                return {
                    .value       = mir::expression::Function_reference { *info },
                    .type        = this_expression.type,
                    .source_view = this_expression.source_view
                };
            }
            else {
                context.error(this_expression.source_view, { "Unrecognized identifier" });
            }
        }

        auto operator()(hir::expression::Type_cast& cast) -> mir::Expression {
            if (cast.kind == ast::expression::Type_cast::Kind::ascription) {
                constraint_set.equality_constraints.push_back({
                    .left  = cast.expression->type,
                    .right = context.resolve_type(*cast.target, scope, space),
                    .constrainer {
                        cast.target->source_view,
                        "The ascribed type is {1}"
                    },
                    .constrained {
                        cast.expression->source_view,
                        "But the actual type is {0}"
                    }
                });
                return recurse(*cast.expression);
            }
            else {
                bu::todo();
            }
        }

        template <class T>
        auto operator()(T&) -> mir::Expression {
            bu::abort(typeid(T).name());
        }
    };

}


auto resolution::Context::resolve_expression(hir::Expression& expression, Scope& scope, Namespace& space)
    -> bu::Pair<Constraint_set, mir::Expression>
{
    Constraint_set constraint_set;

    // Default capacities chosen for no particular reason
    constraint_set.equality_constraints.reserve(256);
    constraint_set.instance_constraints.reserve(128);

    auto value = Expression_resolution_visitor {
        .context         = *this,
        .scope           = scope,
        .space           = space,
        .constraint_set  = constraint_set,
        .this_expression = expression
    }.recurse(expression);

    return {
        .first  = std::move(constraint_set),
        .second = std::move(value)
    };
}