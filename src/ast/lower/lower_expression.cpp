#include "bu/utilities.hpp"
#include "lowering_internals.hpp"


namespace {

    auto lower_function_argument(Lowering_context& context) {
        return [&context](ast::Function_argument const& argument) -> hir::Function_argument {
            return { .expression = context.lower(argument.expression), .name = argument.name };
        };
    }


    struct Expression_lowering_visitor {
        Lowering_context     & context;
        ast::Expression const& this_expression;


        template <class T>
        auto operator()(ast::expression::Literal<T> const& literal) -> hir::Expression {
            return {
                .value = hir::expression::Literal<T> { literal.value },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(ast::expression::Array_literal const& literal) -> hir::Expression {
            return {
                .value = hir::expression::Array_literal {
                    bu::map(literal.elements, context.lower())
                },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(ast::expression::Variable const& variable) -> hir::Expression {
            return {
                .value = variable,
                .source_view = this_expression.source_view
            };
        }

        auto operator()(ast::expression::Tuple const& tuple) -> hir::Expression {
            return {
                .value = hir::expression::Tuple {
                    bu::map(tuple.elements, context.lower())
                },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(ast::expression::Conditional const& conditional) -> hir::Expression {
            auto false_branch = conditional.false_branch
                              . transform(context.lower())
                              . value_or(context.node_context.unit_value);

            if (auto* const let = std::get_if<ast::expression::Conditional_let>(&conditional.condition->value)) {
                return {
                    .value = hir::expression::Match {
                        .cases {
                            hir::expression::Match::Case {
                                .pattern = let->pattern,
                                .expression = context.lower(conditional.true_branch)
                            },
                            hir::expression::Match::Case {
                                .pattern = ast::Pattern {
                                    .value = ast::pattern::Wildcard {},
                                    .source_view = bu::hole()
                                },
                                .expression = std::move(false_branch)
                            }
                        },
                        .expression = context.lower(let->initializer)
                    },
                    .source_view = this_expression.source_view
                };
            }
            else {
                return {
                    .value = hir::expression::Conditional {
                        .condition = context.lower(conditional.condition),
                        .true_branch = context.lower(conditional.true_branch),
                        .false_branch = std::move(false_branch)
                    },
                    .source_view = this_expression.source_view
                };
            }
        }

        auto operator()(ast::expression::Match const& match) -> hir::Expression {
            auto const lower_match_case = [this](ast::expression::Match::Case const& match_case)
                -> hir::expression::Match::Case
            {
                return { .pattern = match_case.pattern, .expression = context.lower(match_case.expression) };
            };

            return {
                .value = hir::expression::Match {
                    .cases = bu::map(match.cases, lower_match_case),
                    .expression = context.lower(match.expression)
                },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(ast::expression::Block const& block) -> hir::Expression {
            return {
                .value = hir::expression::Block {
                    .side_effects = bu::map(block.side_effects, context.lower()),
                    .result = block.result.transform(bu::compose(bu::wrap, context.lower()))
                },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(ast::expression::While_loop const& loop) -> hir::Expression {
            if (auto* const literal = std::get_if<ast::expression::Literal<bool>>(&loop.condition->value)) {
                if (literal->value) {
                    bu::abort("note: use 'loop' instead of 'while true'");
                }
            }

            /*
                while x { y }

                is transformed into

                loop {
                    if x { y } else { break }
                }
            */

            return {
                .value = hir::expression::Loop {
                    .body = hir::Expression {
                        .value = hir::expression::Conditional {
                            .condition    = context.lower(loop.condition),
                            .true_branch  = context.lower(loop.body),
                            .false_branch = hir::expression::Break {}
                        },
                        .source_view = loop.body->source_view
                    }
                },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(ast::expression::Infinite_loop const& loop) -> hir::Expression {
            return {
                .value = hir::expression::Loop { .body = context.lower(loop.body) },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(ast::expression::Invocation const& invocation) -> hir::Expression {
            return {
                .value = hir::expression::Invocation {
                    .arguments = bu::map(invocation.arguments, lower_function_argument(context)),
                    .invocable = context.lower(invocation.invocable)
                },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(ast::expression::Struct_initializer const& initializer) -> hir::Expression {
            decltype(hir::expression::Struct_initializer::member_initializers) initializers;
            initializers.container().reserve(initializer.member_initializers.size());

            for (auto& [name, init] : initializer.member_initializers.span()) {
                initializers.add(bu::copy(name), context.lower(init));
            }

            return {
                .value = hir::expression::Struct_initializer {
                    .member_initializers = std::move(initializers),
                    .type = context.lower(initializer.type)
                },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(ast::expression::Binary_operator_invocation const& invocation) -> hir::Expression {
            return {
                .value = hir::expression::Binary_operator_invocation {
                    .left  = context.lower(invocation.left),
                    .right = context.lower(invocation.right),
                    .op    = invocation.op
                },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(ast::expression::Ret const& ret) -> hir::Expression {
            return {
                .value = hir::expression::Ret {
                    .expression = ret.expression.transform(bu::compose(bu::wrap, context.lower()))
                },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(ast::expression::Size_of const& size_of) -> hir::Expression {
            return {
                .value = hir::expression::Size_of { .type = context.lower(size_of.type) },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(ast::expression::Take_reference const& take) -> hir::Expression {
            return {
                .value = hir::expression::Take_reference {
                    .mutability = take.mutability,
                    .name       = take.name
                },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(ast::expression::Meta const& meta) -> hir::Expression {
            return {
                .value = hir::expression::Meta { .expression = context.lower(meta.expression) },
                .source_view = this_expression.source_view
            };
        }

        auto operator()(ast::expression::Hole const&) -> hir::Expression {
            return {
                .value = hir::expression::Hole {},
                .source_view = this_expression.source_view
            };
        }

        template <class Node>
        auto operator()(Node const&) -> hir::Expression {
            bu::abort(typeid(Node).name());
        }
    };

}


auto Lowering_context::lower(ast::Expression const& expression) -> hir::Expression {
    return std::visit(
        Expression_lowering_visitor {
            .context = *this,
            .this_expression = expression
        },
        expression.value
    );
}