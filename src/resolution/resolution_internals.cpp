#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


auto resolution::Context::error(
    bu::Source_view                    const source_view,
    bu::diagnostics::Message_arguments const arguments) -> void
{
    diagnostics.emit_simple_error(arguments.add_source_info(source, source_view));
}


resolution::Context::Context(
    hir::Node_context        && hir_node_context,
    mir::Node_context        && mir_node_context,
    Namespace::Context       && namespace_context,
    bu::diagnostics::Builder && diagnostics,
    bu::Source               && source
) noexcept
    : hir_node_context  { std::move(hir_node_context) }
    , mir_node_context  { std::move(mir_node_context) }
    , namespace_context { std::move(namespace_context) }
    , diagnostics       { std::move(diagnostics) }
    , source            { std::move(source) } {}


DEFINE_FORMATTER_FOR(resolution::Constraint) {
    return std::visit(bu::Overload {
        [&](resolution::constraint::Equality const& e) {
            return std::format_to(context.out(), "{} = {}", e.left, e.right);
        },
        [&](resolution::constraint::Instance const&) -> std::format_context::iterator {
            bu::todo();
        }
    }, value);
}