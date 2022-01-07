#include "bu/utilities.hpp"


auto main() -> int try {
    bu::print("Hello, world!\n");
}

catch (std::exception const& exception) {
    bu::print<std::cerr>("Error: {}\n", exception.what());
}