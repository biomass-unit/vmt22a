#include "bu/utilities.hpp"
#include "lower.hpp"


namespace {

    class Lowering_context {
        bu::Usize current_name_tag = 0;

        auto fresh_name(char const first) {
            // maybe use a dedicated string pool for compiler-inserted names?
            return lexer::Identifier {
                "{}#{}"_format(first, current_name_tag++),
                bu::Pooled_string_strategy::guaranteed_new_string
            };
        }
    public:
        hir::Node_context& node_context;

        auto fresh_lower_name() -> lexer::Identifier { return fresh_name('x'); }
        auto fresh_upper_name() -> lexer::Identifier { return fresh_name('X'); }

        auto lower(ast::Expression const&) -> hir::Expression;

        auto lower() {
            return [this](ast::Expression const& expression) -> hir::Expression {
                return lower(expression);
            };
        }
    };


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
                              . value_or(context.node_context.get_unit_value());

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

        auto operator()(auto const&) -> hir::Expression {
            bu::todo();
        }
    };


    auto Lowering_context::lower(ast::Expression const& expression) -> hir::Expression {
        return std::visit(
            Expression_lowering_visitor {
                .context = *this,
                .this_expression = expression
            },
            expression.value
        );
    }

}


auto ast::lower(Module&&) -> hir::Module {
    bu::todo();
}