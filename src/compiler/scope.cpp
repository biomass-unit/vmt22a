#include "bu/utilities.hpp"
#include "scope.hpp"


auto compiler::Scope::make_child() -> Scope {
    return { .machine = machine, .parent = this };
}