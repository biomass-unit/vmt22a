#include "bu/utilities.hpp"
#include "lower.hpp"
#include "lowering_internals.hpp"


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
        .source = std::move(module.source)
    };

    Lowering_context context { hir_module.node_context };
    hir_module.definitions = bu::map(module.definitions, context.lower());

    return hir_module;
}