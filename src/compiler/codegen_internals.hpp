#pragma once

#include "bu/utilities.hpp"
#include "ast/ast.hpp"
#include "vm/virtual_machine.hpp"


namespace compiler {

    struct Scope {
        vm::Virtual_machine* machine;
        Scope*               parent;

        auto make_child() -> Scope;
    };

    struct Codegen_context {
        vm::Virtual_machine machine;
        ast::Module         module;
        ast::Namespace*     space;
        Scope               scope;

        Codegen_context(vm::Virtual_machine&& machine, ast::Module&& module)
            : machine { std::move(machine) }
            , module  { std::move(module) }
            , space   { &this->module.global_namespace }
            , scope   { .machine = &this->machine, .parent = nullptr } {}
    };


    auto size_of(ast::Type&      , Codegen_context&) -> vm::Local_size_type;
    auto type_of(ast::Expression&, Codegen_context&) -> ast::Type&;

}