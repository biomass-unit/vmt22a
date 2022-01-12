#include "bu/utilities.hpp"

#include "lexer/lexer.hpp"
#include "lexer/lexer_test.hpp"
#include "lexer/token_formatting.hpp"

#include "ast/ast.hpp"
#include "ast/ast_formatting.hpp"


namespace {

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
    lexer_repl();
}

catch (std::exception const& exception) {
    bu::print<std::cerr>("Error: {}\n", exception.what());
}