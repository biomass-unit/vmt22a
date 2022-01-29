#include "bu/utilities.hpp"
#include "ast/ast.hpp"


ast::Namespace::Namespace(lexer::Identifier const name) noexcept
    : data { Namespace_data { .parent = nullptr, .name = name } } {}


auto ast::Namespace::make_child(lexer::Identifier const name) noexcept -> Namespace {
    Namespace child { name };
    child->parent = data.operator->();
    return data->children.emplace_back(child);
}

auto ast::Namespace::operator->() noexcept -> bu::Wrapper<Namespace_data> {
    return data;
}