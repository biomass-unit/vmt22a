#pragma once

#include "bu/utilities.hpp"
#include "ast/ast.hpp"


namespace typechecker {

    struct Checked_program {};

    auto typecheck(ast::Module&&) -> Checked_program;

}