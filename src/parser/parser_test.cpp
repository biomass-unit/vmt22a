#include "bu/utilities.hpp"
#include "parser_test.hpp"
#include "ast/ast_formatting.hpp"
#include "internals/parser_internals.hpp"


namespace {

    auto test(std::vector<lexer::Token> tokens, ast::Expression const& expression) -> void {
        tokens.push_back({ .type = lexer::Token::Type::end_of_input });

        lexer::Tokenized_source source {
            bu::Source {
                bu::Source::Mock_tag {},
                bu::string_without_sso()
            },
            std::move(tokens)
        };
        parser::Parse_context context { source };

        auto const result = parser::parse_expression(context);

        if (result) {
            if (*result != expression) {
                bu::abort(
                    std::format(
                        "Parser test case failed, with\n\texpected "
                        "expression: {}\n\tactual expression: {}",
                        expression,
                        *result
                    )
                );
            }
            else if (!context.is_finished()) {
                bu::abort(
                    std::format(
                        "Parser test case succeeded, but with remaining input."
                        "\n\texpected expression: {}\nremaining input: {}",
                        expression,
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
                    expression,
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
                    ast::Tuple {}
                }
            },
            ast::Compound_expression {
                {
                    ast::Literal<char> { 'a' }
                }
            }
        }
    );

    bu::print("Parser tests passed!\n");
}