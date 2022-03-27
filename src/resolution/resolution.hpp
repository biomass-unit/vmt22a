#pragma once

#include "ast/ast.hpp"
#include "ir/ir.hpp"


namespace resolution {

    auto resolve(ast::Module&&) -> ir::Program;

}