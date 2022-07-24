#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    template <auto resolution::Namespace::* table>
    auto do_lookup(resolution::Context& context, resolution::Scope& scope, resolution::Namespace& space, hir::Qualified_name& name)
        -> std::optional<typename std::remove_reference_t<decltype(space.*table)>::Value>
    {
        bool is_absolute = false;

        resolution::Namespace* target = std::visit(bu::Overload {
            [&](std::monostate) {
                return &space;
            },
            [&](hir::Root_qualifier::Global) {
                is_absolute = true;
                return &*context.global_namespace;
            },
            [&](hir::Type& hir_type) -> resolution::Namespace* {
                bu::wrapper auto const type = context.resolve_type(hir_type, scope, space);

                if (auto associated = context.namespace_associated_with(type)) {
                    return &**associated;
                }
                else {
                    context.diagnostics.emit_simple_error({
                        .erroneous_view    = bu::get(type->source_view),
                        .source            = context.source,
                        .message           = "{} does not have an associated namespace",
                        .message_arguments = std::make_format_args(type)
                    });
                }
            }
        }, name.root_qualifier.value);

        if (!name.middle_qualifiers.empty()) {
            bu::todo();
        }

        for (; target; target = target->parent ? &**target->parent : nullptr) {
            if (bu::wrapper auto* const ptr = (target->*table).find(name.primary_name.identifier)) {
                return *ptr;
            }
            else if (is_absolute) {
                bu::todo();
            }
        }

        return std::nullopt;
    }

}


auto resolution::Context::find_typeclass(Scope& scope, Namespace& space, hir::Qualified_name& name)
    -> std::optional<bu::Wrapper<Typeclass_info>>
{
    assert(name.primary_name.is_upper);
    return do_lookup<&Namespace::typeclasses>(*this, scope, space, name);
}

auto resolution::Context::find_type(Scope& scope, Namespace& space, hir::Qualified_name& name)
    -> std::optional<std::variant<bu::Wrapper<Struct_info>, bu::Wrapper<Enum_info>, bu::Wrapper<Alias_info>>>
{
    assert(name.primary_name.is_upper);

    if (auto structure = do_lookup<&Namespace::structures>(*this, scope, space, name)) {
        return *structure;
    }
    else if (auto enumeration = do_lookup<&Namespace::enumerations>(*this, scope, space, name)) {
        return *enumeration;
    }
    else if (auto alias = do_lookup<&Namespace::aliases>(*this, scope, space, name)) {
        return *alias;
    }
    else {
        return std::nullopt;
    }
}

auto resolution::Context::find_function(Scope& scope, Namespace& space, hir::Qualified_name& name)
    -> std::optional<bu::Wrapper<Function_info>>
{
    assert(!name.primary_name.is_upper);
    return do_lookup<&Namespace::functions>(*this, scope, space, name);
}