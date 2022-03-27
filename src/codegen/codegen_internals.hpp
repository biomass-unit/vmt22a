#pragma once

#include "bu/utilities.hpp"
#include "bu/flatmap.hpp"
#include "bu/textual_error.hpp"
#include "ast/ast.hpp"
#include "ir/ir.hpp"
#include "vm/virtual_machine.hpp"


namespace compiler {

    struct Codegen_context {
        vm::Virtual_machine* machine;
        ir::Program          program;

        Codegen_context(vm::Virtual_machine& machine, ir::Program&& program) noexcept
            : machine { &machine }
            , program { std::move(program) } {}
    };

}