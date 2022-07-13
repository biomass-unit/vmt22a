#include "bu/utilities.hpp"
#include "lower.hpp"
#include "lowering_internals.hpp"


auto Lowering_context::lower(ast::Qualified_name const& name) -> hir::Qualified_name {
    return {
        .middle_qualifiers = name.middle_qualifiers,
        .root_qualifier = std::visit(bu::Overload {
            [](std::monostate)                -> hir::Root_qualifier { return {}; },
            [](ast::Root_qualifier::Global)   -> hir::Root_qualifier { return { .value = hir::Root_qualifier::Global {} }; },
            [this](ast::Type const& type)     -> hir::Root_qualifier { return { .value = lower(type) }; }
        }, name.root_qualifier.value),
        .primary_name = name.primary_name,
    };
}

auto Lowering_context::lower(ast::Class_reference const& reference) -> hir::Class_reference {
    return {
        .template_arguments = bu::hole(),
        .name = lower(reference.name),
        .source_view = reference.source_view,
    };
}


auto ast::lower(Module&& module) -> hir::Module {
    hir::Module hir_module {
        .node_context {
            .wrapper_context {
                bu::Wrapper_context<hir::Expression> { module.node_context.arena_size<ast::Expression>() },
                bu::Wrapper_context<hir::Type>       { module.node_context.arena_size<ast::Type>() },
                bu::Wrapper_context<hir::Pattern>    { module.node_context.arena_size<ast::Pattern>() },
                bu::Wrapper_context<hir::Definition> { module.node_context.arena_size<ast::Definition>() }
            }
        },
        .diagnostics = std::move(module.diagnostics),
        .source = std::move(module.source)
    };

    Lowering_context context {
        hir_module.node_context,
        hir_module.diagnostics,
        hir_module.source
    };
    hir_module.definitions = bu::map(module.definitions, context.lower());

    return hir_module;
}