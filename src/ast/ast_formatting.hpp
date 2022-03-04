#pragma once

#include "bu/utilities.hpp"
#include "ast.hpp"


DECLARE_FORMATTER_FOR(ast::Expression);
DECLARE_FORMATTER_FOR(ast::Pattern);
DECLARE_FORMATTER_FOR(ast::Type);

DECLARE_FORMATTER_FOR(ast::definition::Function);
DECLARE_FORMATTER_FOR(ast::definition::Function_template);

DECLARE_FORMATTER_FOR(ast::definition::Struct);
DECLARE_FORMATTER_FOR(ast::definition::Struct_template);

DECLARE_FORMATTER_FOR(ast::definition::Data);
DECLARE_FORMATTER_FOR(ast::definition::Data_template);

DECLARE_FORMATTER_FOR(ast::definition::Alias);
DECLARE_FORMATTER_FOR(ast::definition::Alias_template);

DECLARE_FORMATTER_FOR(ast::definition::Instantiation);
DECLARE_FORMATTER_FOR(ast::definition::Instantiation_template);

DECLARE_FORMATTER_FOR(ast::definition::Typeclass);

DECLARE_FORMATTER_FOR(ast::Namespace);

DECLARE_FORMATTER_FOR(ast::Module);

DECLARE_FORMATTER_FOR(ast::Qualified_name);