#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    auto resolve_function_parameters(
        resolution::Context                    & context,
        std::span<hir::Function_parameter> const hir_parameters,
        bu::Wrapper<resolution::Namespace> const home_namespace) -> bu::Pair<resolution::Scope, std::vector<mir::Function_parameter>>
    {
        resolution::Scope                    signature_scope { context };
        std::vector<mir::Function_parameter> mir_parameters;

        mir_parameters.reserve(hir_parameters.size());

        for (hir::Function_parameter& parameter : hir_parameters) {
            bu::wrapper auto const parameter_type =
                context.resolve_type(parameter.type, signature_scope, *home_namespace);

            if (parameter.default_value.has_value()) { // FIX
                auto [constraints, default_value] =
                    context.resolve_expression(*parameter.default_value, signature_scope, *home_namespace);

                constraints.equality_constraints.emplace_back(default_value.type, parameter_type);
                context.unify(constraints);
            }

            if (auto* const name = std::get_if<hir::pattern::Name>(&parameter.pattern.value)) {
                signature_scope.bind_variable(
                    name->identifier,
                    {
                        .type        = parameter_type,
                        .mutability  = name->mutability,
                        .source_view = parameter.pattern.source_view,
                    }
                );
                mir_parameters.emplace_back(mir::Pattern { .value = mir::pattern::Wildcard {} }, parameter_type); // FIX
            }
            else {
                bu::todo();
            }
        }

        return { std::move(signature_scope), std::move(mir_parameters) };
    }


    auto compute_function_signature(resolution::Context& context, resolution::Function_info& info) noexcept {
        return [&](hir::definition::Function& function) -> mir::Function::Signature& {
            if (info.state == resolution::Definition_state::currently_on_resolution_stack) {
                bu::abort();
            }
            info.state = resolution::Definition_state::currently_on_resolution_stack;

            auto [signature_scope, parameters] =
                resolve_function_parameters(context, function.parameters, info.home_namespace);


            if (function.return_type.has_value()) {
                bu::wrapper auto const return_type =
                    context.resolve_type(*function.return_type, signature_scope, *info.home_namespace);

                auto& partially_resolved = info.value.emplace<resolution::Partially_resolved_function>(
                    mir::Function::Signature {
                        .parameters  = std::move(parameters),
                        .return_type = return_type,
                    },
                    std::move(function.body),
                    function.name
                    );
                info.state = resolution::Definition_state::unresolved;
                return partially_resolved.resolved_signature;
            }
            else {
                auto [constraints, body] = context.resolve_expression(function.body, signature_scope, *info.home_namespace);
                context.unify(constraints);

                bu::wrapper auto const return_type = body.type;

                auto& resolved = info.value.emplace<mir::Function>(
                    mir::Function::Signature {
                        .parameters  = std::move(parameters),
                        .return_type = return_type
                    },
                    std::move(body)
                );
                info.state = resolution::Definition_state::resolved;
                return resolved.signature;
            }
        };
    }

}


auto resolution::Context::resolve_function_signature(Function_info& info)
    -> mir::Function::Signature&
{
    return std::visit(bu::Overload {
        compute_function_signature(*this, info),
        std::mem_fn(&Partially_resolved_function::resolved_signature),
        std::mem_fn(&mir::Function::signature)
    }, info.value);
}