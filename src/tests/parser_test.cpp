#include "bu/utilities.hpp"

#ifdef NDEBUG

auto run_parser_tests() -> void {} // No testing in release

#else

#include "ast/ast_formatting.hpp"
#include "parser/internals/parser_internals.hpp"


namespace {

    auto test(std::vector<lexer::Token> tokens, ast::Expression const& expected_expression) -> void {
        tokens.push_back({ .type = lexer::Token::Type::end_of_input });

        lexer::Tokenized_source source {
            bu::Source {
                bu::Source::Mock_tag {},
                bu::string_without_sso()
            },
            std::move(tokens)
        };
        parser::Parse_context context { source };

        std::optional<ast::Expression> parse_result;

        try {
            parse_result = parser::parse_expression(context);
        }
        catch (std::exception const& exception) {
            bu::abort(
                std::format(
                    "Parser test case failed, with\n\texpected"
                    " expression: {}\n\tthrown exception: {}",
                    expected_expression,
                    exception.what()
                )
            );
        }

        if (parse_result) {
            if (*parse_result != expected_expression) {
                bu::abort(
                    std::format(
                        "Parser test case failed, with\n\texpected "
                        "expression: {}\n\tactual expression: {}",
                        expected_expression,
                        *parse_result
                    )
                );
            }
            else if (!context.is_finished()) {
                bu::abort(
                    std::format(
                        "Parser test case succeeded, but with remaining input."
                        "\n\texpected expression: {}\nremaining input: {}",
                        expected_expression,
                        std::vector<lexer::Token> {
                            context.pointer,
                            source.tokens.data() + source.tokens.size()
                        }
                    )
                );
            }
        }
        else {
            bu::abort(
                std::format(
                    "Parser test case failed, with\n\texpected "
                    "expression: {}\n\tinput tokens: {}",
                    expected_expression,
                    source.tokens
                )
            );
        }
    }

}


auto run_parser_tests() -> void {
    assert(lexer::Identifier::string_count() > 16 // should probably work maybe
        && "Parser tests may only be run after the lexer tests have also been run");

    using namespace lexer::literals;
    using namespace bu::literals;

    using lexer::Token;
    using Type = Token::Type;

    test
    (
        {
            Token { 4.2e3, Type::floating }
        },
        ast::Literal<bu::Float> { 4.2e3 }
    );

    test
    (
        {
            Token { .type = Type::if_ },
            Token { false, Type::boolean },
            Token { .type = Type::brace_open },
                Token { true, Type::boolean },
                Token { .type = Type::semicolon },
                Token { .type = Type::paren_open },
                Token { .type = Type::paren_close },
            Token { .type = Type::brace_close },
            Token { .type = Type::else_ },
            Token { .type = Type::brace_open },
                Token { 'a', Type::character },
            Token { .type = Type::brace_close },
        },
        ast::Conditional {
            ast::Literal<bool> { false },
            ast::Compound_expression {
                {
                    ast::Literal<bool> { true },
                    ast::unit_value
                }
            },
            ast::Literal { static_cast<bu::Char>('a') }
        }
    );

    test
    (
        {
            Token { .type = Type::for_ },
            Token { "x"_id, Type::lower_name },
            Token { .type = Type::in },
            Token { lexer::String { "hello"sv }, Type::string },
            Token { .type = Type::brace_open },
            Token { .type = Type::brace_close }
        },
        ast::For_loop {
            ast::pattern::Name { "x"_id },
            ast::Literal { lexer::String { "hello"sv } },
            ast::unit_value
        }
    );

    test
    (
        {
            Token { 1, Type::integer },
            Token { "*"_id, Type::operator_name },
            Token { 2, Type::integer },
            Token { "+$+"_id, Type::operator_name },
            Token { 3, Type::integer },
            Token { "+"_id, Type::operator_name },
            Token { 4, Type::integer }
        },
        ast::Binary_operator_invocation {
            ast::Expression {
                ast::Binary_operator_invocation {
                    ast::Literal<bu::Isize> { 1 },
                    ast::Literal<bu::Isize> { 2 },
                    "*"_id
                }
            },
            ast::Expression {
                ast::Binary_operator_invocation {
                    ast::Literal<bu::Isize> { 3 },
                    ast::Literal<bu::Isize> { 4 },
                    "+"_id
                }
            },
            "+$+"_id
        }
    );

    test
    (
        {
            Token { 'x', Type::character },
            Token { .type = Type::as },
            Token { "Int"_id, Type::upper_name },
            Token { .type = Type::as },
            Token { "Bool"_id, Type::upper_name },
            Token { "+"_id, Type::operator_name },
            Token { 5, Type::integer }
        },
        ast::Binary_operator_invocation {
            ast::Type_cast {
                ast::Expression {
                    ast::Type_cast {
                        ast::Literal { static_cast<bu::Char>('x') },
                        ast::type::integer
                    }
                },
                ast::type::boolean
            },
            ast::Literal<bu::Isize> { 5 },
            "+"_id
        }
    );

    test
    (
        {
            Token { "x"_id, Type::lower_name },
            Token { .type = Type::dot },
            Token { "y"_id, Type::lower_name },
            Token { .type = Type::dot },
            Token { "f"_id, Type::lower_name },
            Token { .type = Type::paren_open },
            Token { .type = Type::paren_close },
        },
        ast::Member_function_invocation {
            .arguments = {},
            .expression = ast::Member_access {
                ast::Variable { ast::Qualified_name { .identifier = "x"_id } },
                "y"_id
            },
            .member_name = "f"_id
        }
    );

    test
    (
        {
            Token { .type = Type::paren_open },
            Token { .type = Type::paren_close },
            Token { .type = Type::dot },
            Token { 1, Type::integer },
            Token { .type = Type::dot },
            Token { 2, Type::integer },
        },
        ast::Tuple_member_access {
            ast::Expression {
                ast::Tuple_member_access {
                    ast::Tuple {},
                    1
                }
            },
            2
        }
    );

    ast::Middle_qualifier::Upper namespace_access_test_vector {
        .template_arguments = std::vector {
            ast::Template_argument {
                ast::Type {
                    ast::type::Typename {
                        ast::Qualified_name {
                            .identifier = "Long"_id
                        }
                    }
                }
            }
        },
        .name = "Vector"_id,
    };
    std::vector<ast::Middle_qualifier> namespace_access_test_qualifiers;

    namespace_access_test_qualifiers.emplace_back(ast::Middle_qualifier::Lower { "std"_id });
    namespace_access_test_qualifiers.emplace_back(std::move(namespace_access_test_vector));

    // ^^^ this isn't done inline because it caused an ICE on MSVC for some reason

    test
    (
        {
            Token { .type = Type::let },
            Token { .type = Type::underscore },
            Token { .type = Type::colon },
            Token { "std"_id, Type::lower_name },
            Token { .type = Type::double_colon },
            Token { "Vector"_id, Type::upper_name },
            Token { .type = Type::bracket_open },
            Token { "Long"_id, Type::upper_name },
            Token { .type = Type::bracket_close },
            Token { .type = Type::double_colon },
            Token { "Element"_id, Type::upper_name },
            Token { .type = Type::equals },
            Token { 5, Type::integer }
        },
        ast::Let_binding {
            .pattern = ast::pattern::Wildcard {},
            .initializer = ast::Literal<bu::Isize> { 5 },
            .type = ast::type::Typename {
                ast::Qualified_name {
                    .qualifiers     = std::move(namespace_access_test_qualifiers),
                    .identifier     = "Element"_id
                }
            }
        }
    );
}

#endif