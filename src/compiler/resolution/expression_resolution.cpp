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


        template <class T>
        auto operator()(ast::expression::Literal<T>& literal) -> ir::Expression {
            return {
                .value = literal,
                .type = ir::Type {
                    .value = ir::type::Primitive<T> {}, // fix
                    .size = sizeof(T)
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
                        .type = initializer.type,
                        .is_mutable = pattern->mutability.type == ast::Mutability::Type::mut
                    }
                );
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