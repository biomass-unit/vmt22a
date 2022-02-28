#pragma once

#include "bu/utilities.hpp"
#include "ast/ast.hpp"
#include "vm/virtual_machine.hpp"
#include "scope.hpp"


struct Codegen_context {
    vm::Virtual_machine machine;
    ast::Module         module;
    ast::Namespace*     space;
    compiler::Scope*    scope;
};