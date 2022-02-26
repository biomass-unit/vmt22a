#include "bu/utilities.hpp"
#include "ast/ast.hpp"


ast::Namespace::Namespace(lexer::Identifier const name) noexcept
    : parent { nullptr }
    , name   { name    } {}


auto ast::Namespace::make_child(lexer::Identifier const child_name) noexcept -> Namespace* {
    Namespace child { child_name };
    child.parent = this;
    return children.add(bu::copy(child_name), std::move(child));
}