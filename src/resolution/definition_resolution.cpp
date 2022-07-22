#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


auto resolution::Context::resolve_function_signature(Function_info& info) -> mir::Function::Signature& {
    if (info.state == Definition_state::currently_on_resolution_stack) {
        bu::abort();
    }

    return std::visit(bu::Overload {
        [&](hir::definition::Function& function) -> mir::Function::Signature& {
            info.state = Definition_state::currently_on_resolution_stack;

            Scope                                signature_scope { *this };
            std::vector<mir::Function_parameter> parameters;
            parameters.reserve(function.parameters.size());

            for (hir::Function_parameter& parameter : function.parameters) {
                bu::wrapper auto const parameter_type =
                    resolve_type(parameter.type, signature_scope, *info.home_namespace);

                if (parameter.default_value.has_value()) { // FIX
                    auto [constraints, default_value] =
                        resolve_expression(*parameter.default_value, signature_scope, *info.home_namespace);

                    constraints.equality_constraints.emplace_back(default_value.type, parameter_type);
                    unify(constraints);
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
                    parameters.emplace_back(mir::Pattern { .value = mir::pattern::Wildcard {} }, parameter_type); // FIX
                }
                else {
                    bu::todo();
                }
            }

            if (function.return_type.has_value()) {
                bu::wrapper auto return_type =
                    resolve_type(*function.return_type, signature_scope, *info.home_namespace);

                auto& partially_resolved = info.value.emplace<resolution::Partially_resolved_function>(
                    mir::Function::Signature {
                        .parameters  = std::move(parameters),
                        .return_type = return_type,
                    },
                    std::move(function.body),
                    function.name
                );
                info.state = Definition_state::unresolved;
                return partially_resolved.resolved_signature;
            }
            else {
                auto [constraints, body] = resolve_expression(function.body, signature_scope, *info.home_namespace);
                unify(constraints);

                bu::wrapper auto const return_type = body.type;

                auto& resolved = info.value.emplace<mir::Function>(
                    mir::Function::Signature {
                        .parameters  = std::move(parameters),
                        .return_type = return_type
                    },
                    std::move(body)
                );
                info.state = Definition_state::resolved;
                return resolved.signature;
            }
        },
        [](resolution::Partially_resolved_function& function) -> mir::Function::Signature& {
            return function.resolved_signature;
        },
        [](mir::Function& function) -> mir::Function::Signature& {
            return function.signature;
        },
    }, info.value);
}