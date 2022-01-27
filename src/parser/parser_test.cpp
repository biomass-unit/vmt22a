#include "bu/utilities.hpp"
#include "parser_test.hpp"

#ifdef NDEBUG

auto parser::run_tests() -> void {} // No testing in release

#else

#include "ast/ast_formatting.hpp"
#include "internals/parser_internals.hpp"


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


auto parser::run_tests() -> void {
    bu::Wrapper_context<ast::Expression> expression_context { 32 };
    bu::Wrapper_context<ast::Pattern   >    pattern_context { 32 };
    bu::Wrapper_context<ast::Type      >       type_context { 32 };

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
            ast::Literal<char> { 'a' }
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

    bu::print("Parser tests passed!\n");
}

#endif