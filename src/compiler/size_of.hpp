#pragma once

#include "ast/ast.hpp"
#include "vm/virtual_machine.hpp"


namespace compiler {

    auto size_of(ast::Type&, ast::Namespace&) -> vm::Local_size_type;

}