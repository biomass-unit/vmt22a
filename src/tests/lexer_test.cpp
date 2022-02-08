#include "bu/utilities.hpp"


#ifdef NDEBUG

auto run_lexer_tests() -> void {} // No testing in release

#else

#include "lexer/lexer.hpp"
#include "lexer/token_formatting.hpp"


namespace {

    auto test(std::string_view const text, std::initializer_list<lexer::Token::Type> types) -> void {
        bu::Source source { bu::Source::Mock_tag {}, std::string { text } };
        std::vector<lexer::Token> tokens;

        try {
            tokens = lexer::lex(std::move(source)).tokens;
            tokens.pop_back(); // get rid of the end_of_input token
        }
        catch (std::exception const& exception) {
            bu::abort(
                std::format(
                    "Lexer test case failed, with\n\tsource: '{}'\n\tthrown exception: {}",
                    source.name(),
                    exception.what()
                )
            );
        }

        if (!std::ranges::equal(tokens, types, {}, &lexer::Token::type)) {
            bu::abort(
                std::format(
                    "Lexer test case failed, with\n\tsource: '{}'\n\t"
                    "expected token types: {}\n\tactual tokens: {}",
                    text,
                    std::vector(types), // there is no formatter for std::initializer_list
                    tokens
                )
            );
        }
    }

}


auto run_lexer_tests() -> void {
    using enum lexer::Token::Type;

    test("50 23.4 0xdeadbeef", { integer, floating, integer });
    test("0.3e-5 3e3 -0. -0.2E5", { floating, integer, floating, floating });
    test(".0.0, 0.0", { dot, integer, dot, integer, comma, floating });
    test("\n::\t,;(--? @#", { double_colon, comma, semicolon, paren_open, operator_name, operator_name });
    test(". /* , /*::*/! */ in /**/ / //", { dot, in, operator_name });

    test(",.[}\tmatch::", { comma, dot, bracket_open, brace_close, match, double_colon });
    test("for;forr(for2", { for_, semicolon, lower_name, paren_open, lower_name });
    test("x1 _ wasd,3"  , { lower_name, underscore, lower_name, comma, integer });
    test("a<$>_:\nVec"  , { lower_name, operator_name, underscore, colon, upper_name });

    test("\"test\\t\\\",\", 'a', '\\\\'", { string, comma, character, comma, character });
}

#endif