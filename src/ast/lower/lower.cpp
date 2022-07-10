#include "bu/utilities.hpp"
#include "lower.hpp"


auto ast::lower(Module&& module) -> hir::Module {
    hir::Node_context hir_node_context {
        .wrapper_context {
            bu::Wrapper_context<hir::Expression> { module.node_context.arena_size<ast::Expression>() },
            bu::Wrapper_context<hir::Type>       { module.node_context.arena_size<ast::Type>() },
            bu::Wrapper_context<hir::Pattern>    { module.node_context.arena_size<ast::Pattern>() },
            bu::Wrapper_context<hir::Definition> { module.node_context.arena_size<ast::Definition>() }
        }
    };
    std::ignore = hir_node_context;
    bu::todo();
}