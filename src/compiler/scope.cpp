#include "bu/utilities.hpp"
#include "codegen_internals.hpp"


auto compiler::Scope::make_child() & -> Scope {
    return { .machine = machine, .parent = this };
}

auto compiler::Scope::find(lexer::Identifier const name) noexcept -> Binding* {
    if (auto* const binding = bindings.find(name)) {
        return binding;
    }
    else {
        return parent ? parent->find(name) : nullptr;
    }
}