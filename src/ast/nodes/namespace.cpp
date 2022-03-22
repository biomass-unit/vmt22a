#include "bu/utilities.hpp"

#if 0

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


    template <class Variant, auto ast::Namespace::* head, auto ast::Namespace::*... tail>
    auto find_one_of(ast::Namespace& space, lexer::Identifier const identifier)
        -> Variant
    {
        if (auto* const pointer = (space.*head).find(identifier)) {
            return pointer;
        }
        else {
            if constexpr (sizeof...(tail) != 0) {
                return find_one_of<Variant, tail...>(space, identifier);
            }
            else {
                return std::monostate {};
            }
        }
    }

}


auto ast::Namespace::find_upper(Qualified_name& qualified_name)
    -> Upper_variant
{
    static constexpr auto find = find_one_of<
        Upper_variant,
        &ast::Namespace::struct_definitions,
        &ast::Namespace::data_definitions,
        &ast::Namespace::alias_definitions,
        &ast::Namespace::struct_template_definitions,
        &ast::Namespace::data_template_definitions,
        &ast::Namespace::alias_template_definitions,
        &ast::Namespace::class_definitions,
        &ast::Namespace::class_template_definitions
    >;

    auto* const space = apply_qualifiers(this, qualified_name);
    return find(*space, qualified_name.primary_qualifier.identifier);
}


auto ast::Namespace::find_lower(Qualified_name& qualified_name)
    -> Lower_variant
{
    static constexpr auto find = find_one_of<
        Lower_variant,
        &ast::Namespace::function_definitions,
        &ast::Namespace::function_template_definitions
    >;

    auto* const space = apply_qualifiers(this, qualified_name);
    return find(*space, qualified_name.primary_qualifier.identifier);
}

#endif