#pragma once

#include "bu/utilities.hpp"
#include "ast.hpp"


DECLARE_FORMATTER_FOR(ast::Expression);
DECLARE_FORMATTER_FOR(ast::Pattern);
DECLARE_FORMATTER_FOR(ast::Type);

DECLARE_FORMATTER_FOR(ast::definition::Function);
DECLARE_FORMATTER_FOR(ast::definition::Struct);
DECLARE_FORMATTER_FOR(ast::definition::Data);

template <class T>
DECLARE_FORMATTER_FOR_TEMPLATE(ast::definition::Template_definition<T>);

extern template struct std::formatter<ast::definition::Function_template>;
extern template struct std::formatter<ast::definition::Struct_template>;
extern template struct std::formatter<ast::definition::Data_template>;