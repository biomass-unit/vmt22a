#pragma once

#include "ast/ast.hpp"
#include "vm/virtual_machine.hpp"


namespace compiler {

    auto compile(ast::Module&&) -> vm::Virtual_machine;

}