#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Expression_resolution_visitor {
        compiler::Resolution_context& context;
        ast::Expression&              this_expression;

        auto recurse(ast::Expression& expression) -> ir::Expression {
            return compiler::resolve_expression(expression, context);
        }
        auto recurse() noexcept {
            return [this](ast::Expression& expression) -> ir::Expression {
                return recurse(expression);
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