#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    using namespace resolution;

    enum class Lookup {
        relative,
        absolute
    };

    auto apply_qualifiers(
        Context            & context,
        Scope              & scope,
        Namespace          & space,
        hir::Qualified_name& name) -> bu::Pair<Lookup, Namespace*>
    {
        Lookup lookup_strategy = Lookup::relative;

        Namespace* const target = std::visit(bu::Overload {
            [&](std::monostate) {
                return &space;
            },
            [&](hir::Root_qualifier::Global) {
                lookup_strategy = Lookup::absolute;
                return &*context.global_namespace;
            },
            [&](hir::Type& hir_type) {
                bu::wrapper auto const type = context.resolve_type(hir_type, scope, space);

                if (auto associated = context.namespace_associated_with(type)) {
                    return &**associated;
                }
                else {
                    context.error(bu::get(type->source_view), {
                        .message           = "{} does not have an associated namespace",
                        .message_arguments = std::make_format_args(type)
                    });
                }
            }
        }, name.root_qualifier.value);

        if (!name.middle_qualifiers.empty()) {
            bu::todo();
        }

        return { lookup_strategy, target };
    }


    template <class R, auto Namespace::* table, auto Namespace::*... tables>
    auto do_absolute_lookup(Namespace& space, ast::Name const name) -> R {
        if (bu::wrapper auto* const info = (space.*table).find(name.identifier)) {
            return *info;
        }
        else {
            if constexpr (sizeof...(tables) == 0) {
                return std::nullopt;
            }
            else {
                return do_absolute_lookup<R, tables...>(space, name);
            }
        }
    }


    template <
        auto (Context::* lookup_function)(Scope&, Namespace&, hir::Qualified_name&),
        auto Namespace::*... tables
    >
    auto do_lookup(
        Context            & context,
        Scope              & scope,
        Namespace          & space,
        hir::Qualified_name& name)
    {
        auto [strategy, target] = apply_qualifiers(context, scope, space, name);
        using R = decltype((context.*lookup_function)(scope, space, name));

        switch (strategy) {
        case Lookup::absolute:
            return do_absolute_lookup<R, tables...>(space, name.primary_name);
        case Lookup::relative:
        {
            for (; target; target = target->parent ? &**target->parent : nullptr) {
                if (auto info = do_absolute_lookup<R, tables...>(space, name.primary_name)) {
                    return info;
                }
            }
            return static_cast<R>(std::nullopt);
        }
        default:
            std::unreachable();
        }
    }

}


auto resolution::Context::find_typeclass(Scope& scope, Namespace& space, hir::Qualified_name& name)
    -> std::optional<bu::Wrapper<Typeclass_info>>
{
    assert(name.primary_name.is_upper);
    return do_lookup<&Context::find_typeclass, &Namespace::typeclasses>(*this, scope, space, name);
}

auto resolution::Context::find_type(Scope& scope, Namespace& space, hir::Qualified_name& name)
    -> std::optional<std::variant<bu::Wrapper<Struct_info>, bu::Wrapper<Enum_info>, bu::Wrapper<Alias_info>>>
{
    assert(name.primary_name.is_upper);
    return do_lookup<&Context::find_type, &Namespace::structures, &Namespace::enumerations, &Namespace::aliases>(*this, scope, space, name);
}

auto resolution::Context::find_function(Scope& scope, Namespace& space, hir::Qualified_name& name)
    -> std::optional<bu::Wrapper<Function_info>>
{
    assert(!name.primary_name.is_upper);
    return do_lookup<&Context::find_function, &Namespace::functions>(*this, scope, space, name);
}