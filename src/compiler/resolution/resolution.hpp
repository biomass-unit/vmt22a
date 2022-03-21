#pragma once

#include "bu/utilities.hpp"
#include "ast/ast.hpp"
#include "ir.hpp"


namespace compiler {

    auto resolve(ast::Module&&) -> ir::Program;

}