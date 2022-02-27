#include "bu/utilities.hpp"
#include "ast/ast.hpp"


ast::Namespace::Namespace(lexer::Identifier const name) noexcept
    : parent { nullptr }
    , global { this    }
    , name   { name    } {}


auto ast::Namespace::make_child(lexer::Identifier const child_name) noexcept -> Namespace* {
    Namespace child { child_name };
    child.parent = this;
    child.global = global;
    return children.add(bu::copy(child_name), std::move(child));
}


namespace {

    auto root_namespace(ast::Namespace* current, ast::Qualified_name& name)
        -> ast::Namespace*
    {
        auto* root = std::visit(bu::Overload {
            [&](std::monostate) {
                return current; // current namespace or above
            },
            [&](ast::Root_qualifier::Global) {
                return current->global; // drill down from global
            },
            [&](ast::Type&) -> ast::Namespace* {
                bu::unimplemented();
            }
        }, name.root_qualifier->value);

        for (auto& qualifier : name.qualifiers) {
            static_assert(std::variant_size_v<decltype(qualifier.value)> == 2);

            if (auto* upper = std::get_if<ast::Middle_qualifier::Upper>(&qualifier.value)) {
                bu::unimplemented();
            }
            else {
                auto& lower = *std::get_if<ast::Middle_qualifier::Lower>(&qualifier.value);
                if (auto* child = root->children.find(lower.name)) {
                    root = child;
                }
                else {
                    bu::abort(std::format("{} does not have a sub-namespace with the name {}", root->name, lower.name));
                }
            }
        }

        return root;
    }

}


auto ast::Namespace::find_type_or_typeclass(Qualified_name& qualified_name)
    -> std::variant<std::monostate, definition::Struct*, definition::Data*, definition::Typeclass*>
{
    auto* space = root_namespace(this, qualified_name);

    auto* structure = space -> struct_definitions . find(qualified_name.identifier);
    auto* data      = space -> data_definitions   . find(qualified_name.identifier);
    auto* typeclass = space -> class_definitions  . find(qualified_name.identifier);

    if (structure) {
        assert(!data && !typeclass);
        return structure;
    }
    else if (data) {
        assert(!structure && !typeclass);
        return data;
    }
    else if (typeclass) {
        assert(!data && !structure);
        return typeclass;
    }
    else {
        return {};
    }
}


auto ast::Namespace::find_function_or_constructor(Qualified_name& qualified_name)
    -> std::variant<std::monostate, definition::Function*/*, ast::definition::Data::Constructor**/>
{
    auto* space = root_namespace(this, qualified_name);

    auto* function = space->function_definitions.find(qualified_name.identifier);

    // TODO: add support for data ctors

    if (function) {
        return function;
    }
    else {
        return {};
    }
}