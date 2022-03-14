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
            if constexpr (std::same_as<T, bu::Isize>)
                this_expression.type = ast::type::integer;
            else if constexpr (std::same_as<T, bu::Float>)
                this_expression.type = ast::type::floating;
            else if constexpr (std::same_as<T, bu::Char>)
                this_expression.type = ast::type::character;
            else if constexpr (std::same_as<T, bool>)
                this_expression.type = ast::type::boolean;
            else if constexpr (std::same_as<T, lexer::String>)
                this_expression.type = ast::type::string;
            else
                static_assert(bu::always_false<T>);
        }

        auto operator()(ast::Variable& variable) -> void {
            if (variable.name.is_unqualified()) {
                if (auto* binding = context.scope.find(variable.name.primary_qualifier.identifier)) {
                    this_expression.type = binding->type;
                    return;
                }
            }

            using ast::definition::Function;

            std::visit(bu::Overload {
                [&](Function* function) {
                    assert(function->return_type);

                    ast::type::Function type { .return_type = *function->return_type };
                    type.argument_types.reserve(function->parameters.size());
                    std::ranges::copy(
                        function->parameters | std::views::transform(&Function::Parameter::type),
                        std::back_inserter(type.argument_types)
                    );

                    this_expression.type.emplace(std::move(type));
                },
                [&](ast::definition::Function_template*) {
                    bu::unimplemented();
                },
                [&](std::monostate) {
                    throw context.error(
                        std::format(
                            "The name {} does not refer to an expression",
                            variable.name
                        ),
                        this_expression
                    );
                }
            }, context.space->find_lower(variable.name));
        }

        auto operator()(ast::Let_binding&) -> void {
            this_expression.type = ast::type::unit;
            // Fix later
        }

        template <bu::one_of<ast::Array_literal, ast::List_literal> Literal>
        auto operator()(Literal& literal) -> void {
            if (literal.elements.empty()) {
                bu::unimplemented();
            }
            else {
                auto& head = literal.elements.front();
                recurse(head);

                for (auto& element : literal.elements | std::views::drop(1)) {
                    recurse(element);

                    if (*element.type != *head.type) {
                        throw context.error(
                            std::format(
                                "The previous elements had type {}, not {}",
                                *head.type,
                                *element.type
                            ),
                            element
                        );
                    }
                }

                if constexpr (std::same_as<Literal, ast::Array_literal>) {
                    this_expression.type = ast::type::Array {
                        *head.type,
                        ast::Literal { static_cast<bu::Isize>(literal.elements.size()) }
                    };
                }
                else {
                    this_expression.type = ast::type::List { *head.type };
                }
            }
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
                throw context.error(
                    std::format("{} is not invocable", invocable_type),
                    this_expression
                );
            }
        }

        auto operator()(ast::Meta& meta) -> void {
            recurse(meta.expression);
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