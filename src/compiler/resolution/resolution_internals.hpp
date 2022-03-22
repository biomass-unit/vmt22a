#pragma once

#include "bu/utilities.hpp"
#include "ast/ast.hpp"
#include "ir.hpp"


namespace compiler {

    struct Resolution_context {};

    auto resolve_type(ast::Type&, Resolution_context&) -> ir::Type;

}