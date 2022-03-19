#include "bu/utilities.hpp"
#include "compiler.hpp"
#include "codegen_internals.hpp"


auto compiler::compile(ast::Module&&) -> vm::Virtual_machine {
    bu::unimplemented();

    /*vm::Virtual_machine machine {
        .stack = bu::Bytestack { 1'000'000 }
    };

    Codegen_context context { machine, std::move(module) };

    for (auto& [name, function] : context.space->function_definitions.container()) {
        (void)function;
        // register sigs, recurse context.space->children
    }

    return machine;*/
}