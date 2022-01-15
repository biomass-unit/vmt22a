#pragma once

#include "bu/utilities.hpp"
#include "ast/ast.hpp"


namespace parser {

    [[nodiscard]]
    auto parse(std::vector<lexer::Token>&&) -> ast::Module;

}