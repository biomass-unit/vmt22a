#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


resolution::Context::Context(
    hir::Node_context       && hir_node_context,
    mir::Node_context       && mir_node_context,
    Namespace::Context      && namespace_context,
    bu::diagnostics::Builder&& diagnostics,
    bu::Source              && source
) noexcept
    : hir_node_context  { std::move(hir_node_context) }
    , mir_node_context  { std::move(mir_node_context) }
    , namespace_context { std::move(namespace_context) }
    , diagnostics       { std::move(diagnostics) }
    , source            { std::move(source) } {}


auto resolution::Context::error(
    bu::Source_view                    const source_view,
    bu::diagnostics::Message_arguments const arguments) -> void
{
    diagnostics.emit_simple_error(arguments.add_source_info(source, source_view));
}


auto resolution::Context::fresh_general_unification_variable() -> bu::Wrapper<mir::Type> {
    return mir::Type { .value = mir::type::General_variable { .tag { .value = current_type_variable_tag++.get() } } };
}
auto resolution::Context::fresh_integral_unification_variable() -> bu::Wrapper<mir::Type> {
    return mir::Type { .value = mir::type::Integral_variable { .tag { .value = current_type_variable_tag++.get() } } };
}


auto resolution::Context::namespace_associated_with(mir::Type& type)
    -> std::optional<bu::Wrapper<resolution::Namespace>>
{
    if (auto* const structure = std::get_if<mir::type::Structure>(&type.value)) {
        return structure->info->associated_namespace;
    }
    else if (auto* const enumeration = std::get_if<mir::type::Enumeration>(&type.value)) {
        return enumeration->info->associated_namespace;
    }
    else {
        return std::nullopt;
    }
}


DEFINE_FORMATTER_FOR(resolution::Constraint_set) {
    auto out = context.out();

    for (auto const& equality_constraint : value.equality_constraints) {
        std::format_to(out, "{} = {}\n", equality_constraint.left, equality_constraint.right);
    }
    for (auto const& instance_constraint : value.instance_constraints) {
        std::format_to(out, "{}: {}", instance_constraint.type, bu::fmt::delimited_range(instance_constraint.classes, " + "));
    }

    return out;
}