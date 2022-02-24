#pragma once

#include "ast/ast.hpp"


namespace compiler {
    
    auto type_of(ast::Expression&, ast::Namespace&) -> bu::Wrapper<ast::Type>;

}