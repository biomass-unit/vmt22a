#include "bu/utilities.hpp"

#include "lexer/lexer.hpp"
#include "lexer/lexer_test.hpp"
#include "lexer/token_formatting.hpp"

#include "ast/ast.hpp"
#include "ast/ast_formatting.hpp"

#include "parser/parser.hpp"
#include "parser/parser_test.hpp"


namespace {

    [[maybe_unused]]
    auto lexer_repl() -> void {
        std::string string;
        for (;;) {
            bu::print(" >>> ");
            std::getline(std::cin, string);

            if (string.empty()) {
                continue;
            }

            try {
                bu::print("Tokens: {}\n", lexer::lex(string));
            }
            catch (std::exception const& exception) {
                bu::print<std::cerr>("REPL error: {}\n", exception.what());
            }
        }
    }

}


auto main() -> int try {
    lexer::run_tests();
    parser::run_tests();
}

catch (std::exception const& exception) {
    bu::print<std::cerr>("Error: {}\n", exception.what());
}