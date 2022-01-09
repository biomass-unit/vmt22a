#include "bu/utilities.hpp"

#include "lexer/lexer.hpp"
#include "lexer/lexer_test.hpp"


auto main() -> int try {
    lexer::run_tests();
}

catch (std::exception const& exception) {
    bu::print<std::cerr>("Error: {}\n", exception.what());
}