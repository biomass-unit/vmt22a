#include "bu/utilities.hpp"
#include "codegen_internals.hpp"
#include "ast/ast_formatting.hpp"


namespace {

    using namespace compiler;

    struct Type_visitor {
        Codegen_context& context;
        ast::Expression& this_expression;

        auto recurse() {
            return [&](ast::Expression& expression) -> ast::Type& {
                return compiler::type_of(expression, context);
            };
        }
        auto recurse(ast::Expression& expression) -> ast::Type& {
            return recurse()(expression);
        }


        template <class T>
        auto operator()(ast::Literal<T>&) -> void {
            this_expression.type.emplace(ast::type::Primitive<T> {});
        }

        auto operator()(ast::Variable&) -> void {
            bu::unimplemented();
        }

        auto operator()(ast::Invocation& invocation) -> void {
            ast::Type& invocable_type = recurse(invocation.invocable);

            if (auto* function = std::get_if<ast::type::Function>(&invocable_type.value)) {
                auto const expected_count = function->argument_types.size();
                auto const actual_count = invocation.arguments.size();

                if (expected_count == actual_count) {
                    bool const ok = std::ranges::equal(
                        invocation.arguments,
                        function->argument_types,
                        {},
                        bu::compose(recurse(), &ast::Function_argument::expression),
                        bu::dereference
                    );

                    if (!ok) {
                        bu::unimplemented();
                    }

                    this_expression.type.emplace(function->return_type);
                }
                else {
                    bu::unimplemented();
                }
            }
            else {
                bu::unimplemented();
            }
        }

        auto operator()(ast::Meta& meta) -> void {
            std::ignore = recurse(meta.expression);
            this_expression.type = meta.expression->type;
        }

        auto operator()(auto&) -> void {
            bu::abort(std::format("compiler::type_of has not been implemented for {}", this_expression));
        }
    };

}


auto compiler::type_of(ast::Expression& expression, Codegen_context& context) -> ast::Type& {
    if (!expression.type) {
        std::visit(Type_visitor { context, expression }, expression.value);
        assert(expression.type);
    }
    return *expression.type;
}