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

    auto apply_qualifiers(ast::Namespace* current, ast::Qualified_name& name)
        -> ast::Namespace*
    {
        auto* space = std::visit(bu::Overload {
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

        for (auto& qualifier : name.middle_qualifiers) {
            static_assert(std::variant_size_v<decltype(qualifier.value)> == 2);

            if (auto* upper = qualifier.upper()) {
                bu::unimplemented();
            }
            else {
                if (auto* child = space->children.find(qualifier.lower()->name)) {
                    space = child;
                }
                else {
                    bu::abort(
                        std::format(
                            "{} does not have a sub-namespace with the name {}",
                            space->name,
                            qualifier.lower()->name
                        )
                    );
                }
            }
        }

        return space;
    }

}


auto ast::Namespace::find_upper(Qualified_name& qualified_name)
    -> Upper_variant
{
    auto* const space = apply_qualifiers(this, qualified_name);

    auto* structure = space -> struct_definitions . find(qualified_name.primary_qualifier.identifier);
    auto* data      = space -> data_definitions   . find(qualified_name.primary_qualifier.identifier);
    auto* alias     = space -> alias_definitions  . find(qualified_name.primary_qualifier.identifier);
    auto* typeclass = space -> class_definitions  . find(qualified_name.primary_qualifier.identifier);

    if (structure) {
        assert(!data && !alias && !typeclass);
        return structure;
    }
    else if (data) {
        assert(!structure && !alias && !typeclass);
        return data;
    }
    else if (alias) {
        assert(!structure && !data && !typeclass);
        return alias;
    }
    else if (typeclass) {
        assert(!data && !alias && !structure);
        return typeclass;
    }
    else {
        return {};
    }
}


auto ast::Namespace::find_lower(Qualified_name& qualified_name)
    -> Lower_variant
{
    auto* const space = apply_qualifiers(this, qualified_name);

    auto* function = space->function_definitions.find(qualified_name.primary_qualifier.identifier);

    // TODO: add support for data ctors

    if (function) {
        return function;
    }
    else {
        return {};
    }
}