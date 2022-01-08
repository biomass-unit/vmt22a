#include "bu/utilities.hpp"

#include "lexer/token.hpp"


auto main() -> int try {
    bu::unimplemented();
}

catch (std::exception const& exception) {
    bu::print<std::cerr>("Error: {}\n", exception.what());
}