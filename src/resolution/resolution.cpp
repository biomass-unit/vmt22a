#include "bu/utilities.hpp"
#include "resolution.hpp"
#include "resolution_internals.hpp"
#include "lir/lir.hpp"


auto resolution::Context::fresh_type_variable(mir::type::Variable::Kind const kind) -> mir::Type {
    return { mir::type::Variable { .tag { .value = current_type_variable_tag++.get() }, .kind = kind } };
}


namespace {

    auto register_top_level_definitions(hir::Module&, resolution::Context&) -> resolution::Namespace {
        bu::todo();
    }

}


auto resolution::resolve(hir::Module&& module) -> Module {
    mir::Node_context node_context;
    resolution::Context context { node_context };

    [[maybe_unused]] auto global_namespace = register_top_level_definitions(module, context);
    bu::todo();
}