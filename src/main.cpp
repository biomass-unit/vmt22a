#include "bu/utilities.hpp"

#include "lexer/lexer.hpp"
#include "lexer/lexer_test.hpp"
#include "lexer/token_formatting.hpp"

#include "ast/ast.hpp"
#include "ast/ast_formatting.hpp"


auto main() -> int try {
    lexer::run_tests();
}

catch (std::exception const& exception) {
    bu::print<std::cerr>("Error: {}\n", exception.what());
}