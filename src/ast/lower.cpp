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
        auto fresh_lower_name() -> lexer::Identifier { return fresh_name('x'); }
        auto fresh_upper_name() -> lexer::Identifier { return fresh_name('X'); }

        auto lower(ast::Expression const&) -> hir::Expression;
    };


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