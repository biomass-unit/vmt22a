#include "bu/utilities.hpp"
#include "lexer_test.hpp"
#include "token_formatting.hpp"


namespace {

    auto test(std::string_view source, std::initializer_list<lexer::Token::Type> types) -> void {
        std::vector<lexer::Token> tokens;

        try {
            tokens = lexer::lex(source);
            tokens.erase(tokens.end() - 1); // get rid of the end_of_input token
        }
        catch (std::exception const& exception) {
            bu::abort(
                std::format(
                    "Lexer test case failed, with\n\tsource: '{}'\n\tthrown exception: {}",
                    source,
                    exception.what()
                )
            );
        }

        if (!std::ranges::equal(tokens, types, {}, &lexer::Token::type)) {
            bu::abort(
                std::format(
                    "Lexer test case failed, with\n\tsource: '{}'\n\t"
                    "expected token types: {}\n\tactual tokens: {}",
                    source,
                    std::vector(types), // there is no formatter for std::initializer_list
                    tokens
                )
            );
        }
    }

}


auto lexer::run_tests() -> void {
    using enum lexer::Token::Type;

    test("50 23.4 0xdeadbeef", { integer, floating, integer });
    test("\n::\t,;(--? @#", { double_colon, comma, semicolon, paren_open, operator_name, operator_name });
    test(". /* , /*::*/! */ in /**/ / //", { dot, in, operator_name });

    test(",.[}\tmatch::", { comma, dot, bracket_open, brace_close, match, double_colon });
    test("for;forr(for2", { for_, semicolon, lower_name, paren_open, lower_name });
    test("x1 _ wasd,3"  , { lower_name, underscore, lower_name, comma, integer });
    test("a<$>_:\nVec"  , { lower_name, operator_name, underscore, colon, upper_name });


    bu::print("Lexer tests passed!\n");
}