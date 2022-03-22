#pragma once

#include "bu/utilities.hpp"
#include "ast.hpp"


DECLARE_FORMATTER_FOR(ast::Expression);
DECLARE_FORMATTER_FOR(ast::Pattern);
DECLARE_FORMATTER_FOR(ast::Type);
DECLARE_FORMATTER_FOR(ast::Definition);

DECLARE_FORMATTER_FOR(ast::Module);

DECLARE_FORMATTER_FOR(ast::Qualified_name);