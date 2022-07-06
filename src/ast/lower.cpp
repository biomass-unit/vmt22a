#include "bu/utilities.hpp"
#include "lower.hpp"


namespace {

    struct Lowering_context {
        auto lower(ast::Expression const&) -> hir::Expression;
    };


    struct Expression_lowering_visitor {
        Lowering_context& context;
        bu::Source_view   current_source_view;

        auto expression(hir::Expression::Variant&& value) -> hir::Expression {
            return { .value = std::move(value), .source_view = current_source_view };
        }

        static auto without_source_view(hir::Expression::Variant&& value) -> hir::Expression {
            return { .value = std::move(value), .source_view { {}, {}, {} } };
        }


        template <class T>
        auto operator()(ast::expression::Literal<T> const& literal) -> hir::Expression {
            return expression(hir::expression::Literal<T> { literal.value });
        }

        auto operator()(ast::expression::While_loop const& loop) -> hir::Expression {
            /*
                while x { y }

                is transformed into

                loop {
                    if x { y } else { break }
                }
            */

            return expression(
                hir::expression::Loop {
                    .body = hir::Expression {
                        .value = hir::expression::Conditional {
                            .condition    = context.lower(loop.condition),
                            .true_branch  = context.lower(loop.body),
                            .false_branch = without_source_view(hir::expression::Break {})
                        },
                        .source_view = loop.body->source_view
                    }
                }
            );
        }

        auto operator()(ast::expression::Infinite_loop const& loop) -> hir::Expression {
            return expression(hir::expression::Loop { .body = context.lower(loop.body) });
        }

        auto operator()(auto const&) -> hir::Expression {
            bu::todo();
        }
    };


    auto Lowering_context::lower(ast::Expression const& expression) -> hir::Expression {
        return std::visit(
            Expression_lowering_visitor {
                .context = *this,
                .current_source_view = expression.source_view
            },
            expression.value
        );
    }

}


auto ast::lower(Module&&) -> hir::Module {
    bu::todo();
}