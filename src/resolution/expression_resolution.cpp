#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Expression_resolution_visitor {
        resolution::Context  & context;
        hir::Expression const& this_expression;

        template <class T>
        auto operator()(hir::expression::Literal<T> const& literal) -> mir::Expression::Variant {
            return literal;
        }

        template <class T>
        auto operator()(T const&) -> mir::Expression::Variant {
            bu::abort(typeid(T).name());
        }
    };

}


auto resolution::Context::resolve(hir::Expression& expression) -> mir::Expression {
    return {
        .value = std::visit(
            Expression_resolution_visitor {
                .context         = *this,
                .this_expression = expression
            },
            expression.value
        ),
        .source_view = expression.source_view
    };
}