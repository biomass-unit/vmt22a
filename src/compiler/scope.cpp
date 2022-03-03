#include "bu/utilities.hpp"
#include "codegen_internals.hpp"


auto compiler::Scope::make_child() -> Scope {
    return { .machine = machine, .parent = this };
}