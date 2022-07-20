#include "bu/utilities.hpp"
#include "resolution.hpp"
#include "resolution_internals.hpp"
#include "lir/lir.hpp"


auto resolution::Context::fresh_type_variable(mir::type::Variable::Kind const kind) -> mir::Type {
    return { mir::type::Variable { .tag { .value = current_type_variable_tag++.get() }, .kind = kind } };
}


namespace {

    auto register_top_level_signatures(hir::Module&, resolution::Context&) -> mir::Namespace {
        bu::todo();
    }

}


resolution::Context::Context(
    mir::Node_context       & node_context,
    mir::Namespace::Context & namespace_context,
    bu::diagnostics::Builder& diagnostics
) noexcept
    : node_context      { node_context }
    , namespace_context { namespace_context }
    , diagnostics       { diagnostics  } {}


auto resolution::resolve(hir::Module&& module) -> Module {
    mir::Node_context node_context {
        bu::Wrapper_context<mir::Expression> { module.node_context.arena_size<hir::Expression>() },
        bu::Wrapper_context<mir::Pattern>    { module.node_context.arena_size<hir::Pattern>() },
        bu::Wrapper_context<mir::Type>       { module.node_context.arena_size<hir::Type>() },
    };
    mir::Namespace::Context namespace_context;

    resolution::Context context {
        node_context,
        namespace_context,
        module.diagnostics
    };

    [[maybe_unused]] auto global_namespace = register_top_level_signatures(module, context);
    bu::todo();
}