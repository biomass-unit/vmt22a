#pragma once

#include "bu/utilities.hpp"
#include "bu/flatmap.hpp"
#include "bu/textual_error.hpp"
#include "ast/ast.hpp"
#include "resolution/ir.hpp"
#include "vm/virtual_machine.hpp"


namespace compiler {

    struct Scope {
        struct Binding {
            bu::Wrapper<ast::Type> type;
            vm::Local_offset_type  frame_offset;
        };

        bu::Flatmap<lexer::Identifier, Binding> bindings;
        vm::Virtual_machine*                    machine;
        Scope*                                  parent;

        auto find(lexer::Identifier) noexcept -> Binding*;

        auto make_child() & -> Scope;
    };

    struct Codegen_context {
        vm::Virtual_machine* machine;
        ir::Program          program;
        Scope                scope;

        Codegen_context(vm::Virtual_machine& machine, ir::Program&& program) noexcept
            : machine { &machine }
            , program { std::move(program) }
            , scope   { .machine = &machine, .parent = nullptr } {}
    };

}