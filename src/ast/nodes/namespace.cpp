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


auto ast::Namespace::find_type_or_typeclass(Qualified_name& qualified_name)
    -> std::variant<std::monostate, definition::Struct*, definition::Data*, definition::Typeclass*>
{
    Namespace* space = std::visit(bu::Overload {
        [&](std::monostate) {
            return this; // current namespace or above
        },
        [&](ast::Root_qualifier::Global) {
            return global; // drill down from global
        },
        [&](ast::Type&) -> Namespace* {
            bu::unimplemented();
        }
    }, qualified_name.root_qualifier->value);

    for (auto& qualifier : qualified_name.qualifiers) {
        static_assert(std::variant_size_v<decltype(qualifier.value)> == 2);

        if (auto* upper = std::get_if<ast::Middle_qualifier::Upper>(&qualifier.value)) {
            bu::unimplemented();
        }
        else {
            auto& lower = *std::get_if<ast::Middle_qualifier::Lower>(&qualifier.value);
            if (auto* child = space->children.find(lower.name)) {
                space = child;
            }
            else {
                bu::abort(std::format("{} does not have a sub-namespace with the name {}", space->name, lower.name));
            }
        }
    }

    auto const identifier = qualified_name.identifier;

    auto* structure = space->struct_definitions.find(identifier);
    auto* data      = space->data_definitions  .find(identifier);
    auto* typeclass = space->class_definitions .find(identifier);

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
        return std::monostate {};
    }
}


auto ast::Namespace::find_function_or_constructor(Qualified_name&)
    -> std::variant<std::monostate, definition::Function*, ast::definition::Data::Constructor*>
{
    bu::unimplemented();
}