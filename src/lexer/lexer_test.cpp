#include "bu/utilities.hpp"
#include "bu/formatting.hpp"
#include "lexer_test.hpp"


namespace {

    auto test(std::string_view source, std::initializer_list<lexer::Token::Type> types) -> void {
        if (!std::ranges::equal(lexer::lex(source), types, {}, &lexer::Token::type)) {
            bu::abort("Lexer test case failed!");
        }
    }

}


auto lexer::run_tests() -> void {
    using enum lexer::Token::Type;

    test(",.[}\tmatch::", { comma, dot, bracket_open, brace_close, match, double_colon });
    test("for;forr(for2", { for_, semicolon, lower_name, paren_open, lower_name });
    test("x1 _ wasd,3"  , { lower_name, underscore, lower_name, comma, integer });
    test("a<$>_:\nVec"  , { lower_name, operator_name, underscore, colon, upper_name });
}