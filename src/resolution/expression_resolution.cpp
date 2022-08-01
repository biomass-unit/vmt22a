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


        auto operator()(hir::expression::Literal<bu::Isize>& literal) -> mir::Expression {
            return {
                .value       = mir::expression::Literal { literal.value },
                .type        = context.fresh_integral_unification_variable(),
                .source_view = this_expression.source_view
            };
        }

        auto operator()(hir::expression::Literal<bu::Float>& literal) -> mir::Expression {
            return {
                .value       = mir::expression::Literal { literal.value },
                .type        = mir::Type { .value = mir::type::Floating {}, .source_view = this_expression.source_view },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(hir::expression::Literal<bu::Char>& literal) -> mir::Expression {
            return {
                .value       = mir::expression::Literal { literal.value },
                .type        = mir::Type { .value = mir::type::Character {}, .source_view = this_expression.source_view },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(hir::expression::Literal<bool>& literal) -> mir::Expression {
            return {
                .value       = mir::expression::Literal { literal.value },
                .type        = mir::Type { .value = mir::type::Boolean {}, .source_view = this_expression.source_view },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(hir::expression::Literal<lexer::String>& literal) -> mir::Expression {
            return {
                .value       = mir::expression::Literal { literal.value },
                .type        = mir::Type { .value = mir::type::String {}, .source_view = this_expression.source_view },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(hir::expression::Array_literal& array) -> mir::Expression {
            mir::expression::Array_literal mir_array;
            mir_array.elements.reserve(array.elements.size());

            if (!array.elements.empty()) {
                mir_array.elements.push_back(recurse(array.elements.front()));

                for (bu::Usize i = 1; i != array.elements.size(); ++i) {
                    hir::Expression& previous_element = array.elements[i - 1];
                    hir::Expression& current_element  = array.elements[i];

                    mir_array.elements.push_back(recurse(current_element));

                    constraint_set.equality_constraints.push_back({
                        .left  = mir_array.elements.front().type,
                        .right = mir_array.elements.back().type,
                        .constrainer {
                            array.elements.front().source_view + previous_element.source_view,
                            i == 1 ? "The previous element was of type {0}"
                                   : "The previous elements were of type {0}"
                        },
                        .constrained {
                            current_element.source_view,
                            "But this element is of type {1}"
                        }
                    });
                }
            }

            bu::wrapper auto const element_type = mir_array.elements.empty()
                ? context.fresh_general_unification_variable()
                : mir_array.elements.front().type;

            bu::Isize const array_length = std::ssize(mir_array.elements);

            return {
                .value = std::move(mir_array),
                .type  = mir::Type {
                    .value = mir::type::Array {
                        .element_type = element_type,
                        .length       = mir::Expression {
                            .value       = mir::expression::Literal { array_length },
                            .type        = mir::Type { .value = bu::copy(context.size_type), .source_view = this_expression.source_view },
                            .source_view = this_expression.source_view
                        }
                    },
                    .source_view = this_expression.source_view
                },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(hir::expression::Variable& variable) -> mir::Expression {
            if (variable.name.is_unqualified()) {
                if (auto* const binding = scope.find_variable(variable.name.primary_name.identifier)) {
                    binding->has_been_mentioned = true;
                    return {
                        .value          = mir::expression::Local_variable_reference { variable.name.primary_name },
                        .type           = binding->type,
                        .source_view    = this_expression.source_view,
                        .is_addressable = true
                    };
                }
            }

            if (auto info = context.find_function(scope, space, variable.name)) {
                context.resolve_function(*info);

                return {
                    .value       = mir::expression::Function_reference { *info },
                    .type        = (*info)->function_type,
                    .source_view = this_expression.source_view
                };
            }
            else {
                context.error(this_expression.source_view, { "Unrecognized identifier" });
            }
        }

        auto operator()(hir::expression::Tuple& tuple) -> mir::Expression {
            mir::expression::Tuple mir_tuple;
            mir::type::Tuple mir_tuple_type;

            mir_tuple.elements.reserve(tuple.elements.size());
            mir_tuple_type.types.reserve(tuple.elements.size());

            for (hir::Expression& element : tuple.elements) {
                mir_tuple.elements.push_back(recurse(element));
                mir_tuple_type.types.push_back(mir_tuple.elements.back().type);
            }

            return {
                .value = std::move(mir_tuple),
                .type  = mir::Type {
                    .value       = std::move(mir_tuple_type),
                    .source_view = this_expression.source_view
                },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(hir::expression::Block& block) -> mir::Expression {
            resolution::Scope block_scope = scope.make_child();

            std::vector<mir::Expression> side_effects;
            side_effects.reserve(block.side_effects.size());

            for (hir::Expression& side_effect : block.side_effects) {
                side_effects.push_back(recurse(side_effect, &block_scope));
                constraint_set.equality_constraints.push_back({
                    .left  = side_effects.back().type,
                    .right = mir::type::Tuple {},
                    .constrainer {
                        this_expression.source_view,
                        "The side-effect expressions in a block expression must of the unit type"
                    },
                    .constrained {
                        side_effect.source_view,
                        "But this expression is of type {0}"
                    }
                });
            }

            std::optional<bu::Wrapper<mir::Expression>> block_result;
            if (block.result) {
                block_result = recurse(*block.result, &block_scope);
            }

            bu::wrapper auto const result_type
                = block_result
                ? (*block_result)->type
                : mir::type::Tuple {};

            block_scope.warn_about_unused_bindings();

            return {
                .value = mir::expression::Block {
                    .side_effects = std::move(side_effects),
                    .result       = std::move(block_result)
                },
                .type        = result_type,
                .source_view = this_expression.source_view
            };
        }

        auto operator()(hir::expression::Let_binding& let) -> mir::Expression {
            auto initializer = recurse(let.initializer);

            bu::wrapper auto const type = [&] {
                if (let.type.has_value()) {
                    bu::wrapper auto type = context.resolve_type(*let.type, scope, space);
                    constraint_set.equality_constraints.push_back({
                        .left  = initializer.type,
                        .right = type,
                        .constrainer {
                            (*let.type)->source_view,
                            "The variable is specified to be of type {1}"
                        },
                        .constrained {
                            let.initializer->source_view,
                            "But its initializer is of type {0}"
                        }
                    });
                    return type;
                }
                else {
                    return initializer.type;
                }
            }();

            if (auto* const name = std::get_if<hir::pattern::Name>(&let.pattern->value)) {
                if (name->mutability.parameter_name.has_value()) {
                    bu::todo();
                }

                scope.bind_variable(
                    name->value.identifier,
                    {
                        .type               = type,
                        .mutability         = name->mutability,
                        .has_been_mentioned = false,
                        .source_view        = name->value.source_view,
                    }
                );

                return {
                    .value = mir::expression::Let_binding {
                        .pattern = mir::Pattern {
                            .value = mir::pattern::Name {
                                .value      = name->value,
                                .is_mutable = name->mutability.type == ast::Mutability::Type::mut
                            },
                            .source_view = let.pattern->source_view
                        },
                        .type        = type,
                        .initializer = std::move(initializer),
                    },
                    .type        = mir::Type { .value = mir::type::Tuple {}, .source_view = this_expression.source_view },
                    .source_view = this_expression.source_view
                };
            }
            else {
                bu::todo();
            }
        }

        auto operator()(hir::expression::Type_cast& cast) -> mir::Expression {
            if (cast.kind == ast::expression::Type_cast::Kind::ascription) {
                auto result = recurse(*cast.expression);
                constraint_set.equality_constraints.push_back({
                    .left  = result.type,
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
                return result;
            }
            else {
                bu::todo();
            }
        }

        auto operator()(hir::expression::Invocation& invocation) -> mir::Expression {
            mir::Expression invocable = recurse(invocation.invocable);
            
            // If the invocable is a direct reference to a function, named arguments
            // may be supplied. Otherwise, only positional arguments are allowed.

            if (auto* const function = std::get_if<mir::expression::Function_reference>(&invocable.value)) {
                mir::Function::Signature& signature     = context.resolve_function_signature(function->info);
                auto const&               function_type = bu::get<mir::type::Function>(function->info->function_type->value);

                bu::Usize const argument_count  = invocation.arguments.size();
                bu::Usize const parameter_count = signature.parameters.size();

                if (argument_count != parameter_count) {
                    context.error(this_expression.source_view, {
                        .message             = "The function has {} parameters, but {} arguments were supplied",
                        .message_arguments   = std::make_format_args(parameter_count, argument_count),
                        .help_note           = "The function is of type {}",
                        .help_note_arguments = std::make_format_args(function->info->function_type)
                    });
                }

                std::vector<mir::Expression> arguments;
                arguments.reserve(parameter_count);

                for (bu::Usize i = 0; i != argument_count; ++i) {
                    hir::Function_argument&  argument            = invocation.arguments[i];
                    mir::Expression          argument_expression = recurse(argument.expression);
                    mir::Function_parameter& parameter           = signature.parameters[i];

                    if (argument.name.has_value()) {
                        bu::todo();
                    }

                    constraint_set.equality_constraints.push_back({
                        .left  = argument_expression.type,
                        .right = parameter.type,
                        .constrainer {
                            parameter.type->source_view.has_value()
                                ? parameter.pattern.source_view + *parameter.type->source_view
                                : parameter.pattern.source_view,
                            "The parameter is specified to be of type {1}"
                        },
                        .constrained {
                            argument_expression.source_view,
                            "But the argument is of type {0}"
                        }
                    });

                    arguments.push_back(std::move(argument_expression));
                }

                return {
                    .value = mir::expression::Direct_invocation {
                        .function  { .info = function->info },
                        .arguments = std::move(arguments)
                    },
                    .type        = function_type.return_type,
                    .source_view = this_expression.source_view
                };
            }
            else {
                bu::todo();
            }
        }

        auto operator()(hir::expression::Reference& reference) -> mir::Expression {
            auto             expression      = recurse(reference.expression);
            bu::wrapper auto referenced_type = expression.type;

            if (expression.is_addressable) {
                return {
                    .value = mir::expression::Reference {
                        .mutability = reference.mutability,
                        .expression = std::move(expression)
                    },
                    .type = mir::Type {
                        .value = mir::type::Reference {
                            .mutability      = reference.mutability,
                            .referenced_type = referenced_type
                        },
                        .source_view = this_expression.source_view
                    },
                    .source_view = this_expression.source_view
                };
            }
            else {
                context.error(reference.expression->source_view, { "This expression is not addressable" });
            }
        }

        auto operator()(hir::expression::Dereference& deref) -> mir::Expression {
            auto             expression      = recurse(deref.expression);
            bu::wrapper auto referenced_type = context.fresh_general_unification_variable();

            constraint_set.equality_constraints.push_back({
                .left  = expression.type,
                .right = mir::Type {
                    .value = mir::type::Reference_variable {
                        .referenced_type = referenced_type
                    },
                    .source_view = this_expression.source_view
                },
                .constrainer {
                    this_expression.source_view,
                    "Only expressions of reference types (&T or &mut T) can be dereferenced"
                },
                .constrained {
                    deref.expression->source_view,
                    "But this expression is of type {0}"
                }
            });

            return {
                .value = mir::expression::Dereference {
                    .expression = std::move(expression)
                },
                .type           = referenced_type,
                .source_view    = this_expression.source_view,
                .is_addressable = true
            };
        }

        auto operator()(auto&) -> mir::Expression {
            context.error(this_expression.source_view, { "This expression can not be resolved yet" });
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