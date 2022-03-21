#include "bu/utilities.hpp"
#include "resolution.hpp"


auto compiler::resolve(ast::Module&&) -> ir::Program {
    bu::unimplemented();

    // Perform imports, scope resolution, type
    // checking, turn names into actual references, etc.
}