#include "bu/utilities.hpp"

#include "lexer/lexer.hpp"
#include "lexer/lexer_test.hpp"
#include "lexer/token_formatting.hpp"

#include "bu/wrapper.hpp"


auto main() -> int try {
    //lexer::run_tests();

    bu::Wrapper wrapper { "world" };
    bu::print("Hello, {}!", wrapper);
}

catch (std::exception const& exception) {
    bu::print<std::cerr>("Error: {}\n", exception.what());
}