#pragma once

#include "bu/utilities.hpp"
#include "lexer/lexer.hpp"
#include "ast/ast.hpp"


namespace parser {

    [[nodiscard]]
    auto parse(lexer::Tokenized_source&&) -> ast::Module;

}