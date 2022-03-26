#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Expression_resolution_visitor {
        compiler::Resolution_context& context;
        ast::Expression&              this_expression;

        auto recurse_with(compiler::Resolution_context& context) {
            return [&](ast::Expression& expression) -> ir::Expression {
                return compiler::resolve_expression(expression, context);
            };
        };
        auto recurse(ast::Expression& expression) -> ir::Expression {
            return recurse_with(context)(expression);
        }
        auto recurse() noexcept {
            return [this](ast::Expression& expression) -> ir::Expression {
                return recurse(expression);
            };
        }


        auto error() {}


        template <class T>
        auto operator()(ast::expression::Literal<T>& literal) -> ir::Expression {
            return {
                .value = literal,
                .type = ir::type::dtl::make_primitive<T>
            };
        }

        auto operator()(ast::expression::Tuple& tuple) -> ir::Expression {
            std::vector<ir::Expression> expressions;
            std::vector<bu::Wrapper<ir::Type>> types;
            bu::U16 size       = 0;
            bool    is_trivial = true;

            expressions.reserve(tuple.expressions.size());
            types      .reserve(tuple.expressions.size());

            for (auto& expression : tuple.expressions) {
                auto ir_expr = recurse(expression);

                if (!ir_expr.type->is_trivial) {
                    is_trivial = false;
                }
                size += ir_expr.type->size;

                types.push_back(ir_expr.type);
                expressions.push_back(std::move(ir_expr));
            }

            return {
                .value = ir::expression::Tuple { std::move(expressions) },
                .type = ir::Type {
                    .value      = ir::type::Tuple { std::move(types) },
                    .size       = size,
                    .is_trivial = is_trivial
                }
            };
        }

        auto operator()(ast::expression::Let_binding& let_binding) -> ir::Expression {
            auto initializer = compiler::resolve_expression(let_binding.initializer, context);

            if (let_binding.type) {
                auto explicit_type = compiler::resolve_type(*let_binding.type, context);
                if (explicit_type != *initializer.type) {
                    bu::abort("mismatched types");
                }
            }

            if (auto* const pattern = std::get_if<ast::pattern::Name>(&let_binding.pattern->value)) {
                if (pattern->mutability.type == ast::Mutability::Type::parameterized) {
                    bu::unimplemented();
                }

                context.scope.bindings.add(
                    bu::copy(pattern->identifier),
                    compiler::Binding {
                        .type         = initializer.type,
                        .frame_offset = context.scope.current_frame_offset,
                        .is_mutable   = pattern->mutability.type == ast::Mutability::Type::mut
                    }
                );

                if (!context.is_unevaluated) {
                    context.scope.current_frame_offset += initializer.type->size;
                }
            }
            else {
                bu::unimplemented();
            }

            return {
                .value = ir::expression::Let_binding {
                    .pattern     = std::move(let_binding.pattern),
                    .type        = initializer.type,
                    .initializer = std::move(initializer)
                },
                .type = ir::type::unit
            };
        }

        auto operator()(ast::expression::Variable& variable) -> ir::Expression {
            if (auto const value = context.find_lower(variable.name)) {
                return std::visit(bu::Overload {
                    [&](compiler::Binding* binding) -> ir::Expression {
                        binding->has_been_mentioned = true;

                        if (!context.is_unevaluated && !binding->type->is_trivial) {
                            if (binding->moved_by) {
                                bu::unimplemented();
                            }
                            else {
                                binding->moved_by = &this_expression;
                            }
                        }

                        return {
                            .value = ir::expression::Local_variable {
                                .frame_offset = binding->frame_offset
                            },
                            .type = binding->type
                        };
                    },
                    [&](ast::definition::Function*) -> ir::Expression {
                        bu::unimplemented();
                    },
                    [&](ast::definition::Function_template*) -> ir::Expression {
                        bu::unimplemented();
                    }
                }, *value);
            }
            else {
                bu::abort("undeclared name");
            }
        }

        auto operator()(ast::expression::Take_reference& take_reference) -> ir::Expression {
            if (take_reference.mutability.type == ast::Mutability::Type::parameterized) {
                bu::unimplemented();
            }

            bool const take_mutable_ref =
                take_reference.mutability.type == ast::Mutability::Type::mut;

            if (auto* binding = context.scope.find(take_reference.name)) {
                if (take_mutable_ref && !binding->is_mutable) {
                    bu::unimplemented();
                }

                binding->has_been_mentioned = true;

                return {
                    .value = ir::expression::Reference {
                        .frame_offset = binding->frame_offset
                    },
                    .type = ir::Type {
                        .value = ir::type::Reference {
                            .type = binding->type,
                            .mut  = take_mutable_ref
                        },
                        .size       = sizeof(std::byte*),
                        .is_trivial = true
                    }
                };
            }
            else {
                bu::unimplemented();
            }
        }

        auto operator()(ast::expression::Size_of& size_of) -> ir::Expression {
            bool const is_unevaluated = std::exchange(context.is_unevaluated, true);
            auto type = compiler::resolve_type(size_of.type, context);
            context.is_unevaluated = is_unevaluated;

            return {
                .value = ir::expression::Literal<bu::Isize> {
                    static_cast<bu::Isize>(type.size)
                },
                .type = ir::type::integer
            };
        }

        auto operator()(ast::expression::Member_access_chain& chain) -> ir::Expression {
            auto expression = recurse(chain.expression);
            auto most_recent_type = expression.type;

            bu::U16 offset = 0;

            for (auto& accessor : chain.accessors) {
                std::visit(bu::Overload {
                    [&](bu::Isize const member_index) {
                        if (member_index < 0) {
                            bu::abort("negative tuple member index");
                        }

                        auto const index = static_cast<bu::Usize>(member_index);

                        if (auto* const tuple = std::get_if<ir::type::Tuple>(&most_recent_type->value)) {
                            if (index < tuple->types.size()) {
                                for (auto& type : tuple->types | std::views::take(member_index)) {
                                    offset += type->size;
                                }
                                most_recent_type = tuple->types.at(index);
                            }
                            else {
                                bu::abort("tuple member index out of range");
                            }
                        }
                        else {
                            bu::abort("attempted tuple member access on a non-tuple type");
                        }
                    },
                    [&](lexer::Identifier const member_name) {
                        if (auto* const uds = std::get_if<ir::type::User_defined_struct>(&most_recent_type->value)) {
                            if (auto* const member = uds->structure->members.find(member_name)) {
                                most_recent_type = member->type;
                                offset += member->offset;
                            }
                            else {
                                bu::abort("struct does not contain given member");
                            }
                        }
                        else {
                            bu::abort("attempted member access on non-struct type");
                        }
                    }
                }, accessor);
            }

            return {
                .value = ir::expression::Member_access {
                    std::move(expression),
                    offset
                },
                .type = most_recent_type
            };
        }

        auto operator()(ast::expression::Compound& compound) -> ir::Expression {
            // The parser should convert empty compound expressions into the unit value
            assert(compound.expressions.size() != 0);

            auto child_context = context.make_child_context_with_new_scope();
            auto recurse = recurse_with(child_context); // Shadow the struct member

            std::vector<ir::Expression> side_effects;
            side_effects.reserve(compound.expressions.size() - 1);

            std::ranges::move(
                compound.expressions
                    | std::views::take(compound.expressions.size() - 1)
                    | std::views::transform(recurse),
                std::back_inserter(side_effects)
            );

            auto result = recurse(compound.expressions.back());

            return {
                .value = ir::expression::Compound {
                    std::move(side_effects),
                    std::move(result)
                },
                .type = result.type
            };
        }

        auto operator()(auto&) -> ir::Expression {
            bu::abort(
                std::format(
                    "compiler::resolve_expression has not been implemented for {}",
                    this_expression
                )
            );
        }
    };

}


auto compiler::resolve_expression(ast::Expression& expression, Resolution_context& context) -> ir::Expression {
    return std::visit(Expression_resolution_visitor { context, expression }, expression.value);
}