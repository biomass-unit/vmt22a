#include "bu/utilities.hpp"
#include "type_of.hpp"
#include "ast/ast_formatting.hpp"


namespace {

    struct Type_visitor {
        ast::Namespace&  space;
        ast::Expression& this_expression;

        auto recurse() {
            return [&](ast::Expression& expression) {
                return compiler::type_of(expression, space);
            };
        }
        auto recurse(ast::Expression& expression) {
            return recurse()(expression);
        }


        template <class T>
        auto operator()(ast::Literal<T>&) -> bu::Wrapper<ast::Type> {
            return ast::Type { ast::type::Primitive<T> {} };
        }

        auto operator()(ast::Variable& /*variable*/) -> bu::Wrapper<ast::Type> {
            /*auto it = std::ranges::find(
                space.function_definitions,
                variable.name,
                &ast::definition::Function::name
            );
            if (it != space.function_definitions.end()) {
                if (!it->return_type) {
                    bu::unimplemented();
                }
                auto parameter_types = it->parameters
                                     | std::views::transform(&ast::definition::Function::Parameter::type);
                return ast::type::Function {
                    .argument_types { std::vector(parameter_types.begin(), parameter_types.end()) },
                    .return_type    { *it->return_type }
                };
            }
            else {*/
                bu::unimplemented();
            //}
        }

        auto operator()(ast::Invocation& invocation) -> bu::Wrapper<ast::Type> {
            auto invocable_type = recurse()(invocation.invocable);
            if (auto* function = std::get_if<ast::type::Function>(&invocable_type->value)) {
                auto const expected_count = function->argument_types.size();
                auto const actual_count = invocation.arguments.size();

                if (expected_count == actual_count) {
                    auto const argument_type = [&](ast::Function_argument& arg) noexcept {
                        return recurse()(arg.expression);
                    };
                    if (!std::ranges::equal(invocation.arguments, function->argument_types, {}, argument_type)) {
                        bu::unimplemented();
                    }
                    return function->return_type;
                }
                else {
                    bu::unimplemented();
                }
            }
            else {
                bu::unimplemented();
            }
        }

        auto operator()(ast::Meta& meta) {
            return recurse()(meta.expression);
        }

        auto operator()(auto&) -> bu::Wrapper<ast::Type> {
            bu::abort(std::format("compiler::type_of has not been implemented for {}", this_expression));
        }
    };

}


auto compiler::type_of(ast::Expression& expression, ast::Namespace& space)
    -> bu::Wrapper<ast::Type>
{
    if (!expression.type) {
        expression.type.emplace(
            std::visit(Type_visitor { space, expression }, expression.value)
        );
    }
    return *expression.type;
}