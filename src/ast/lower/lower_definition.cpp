#include "bu/utilities.hpp"
#include "lowering_internals.hpp"


namespace {

    auto lower_template_parameters(
        Lowering_context              & context,
        ast::Template_parameters const& parameters) -> hir::Template_parameters
    {
        auto const lower_template_parameter = [&](ast::Template_parameter const& parameter)
            -> hir::Template_parameter
        {
            return {
                .value = std::visit<hir::Template_parameter::Variant>(bu::Overload {
                    [&context](ast::Template_parameter::Type_parameter const& type_parameter) {
                        return hir::Template_parameter::Type_parameter {
                            .classes = bu::map(type_parameter.classes, context.lower())
                        };
                    },
                    [&](ast::Template_parameter::Value_parameter const& value_parameter) {
                        return hir::Template_parameter::Value_parameter {
                            value_parameter.type.transform(context.lower())
                        };
                    },
                    [](ast::Template_parameter::Mutability_parameter const&) {
                        return hir::Template_parameter::Mutability_parameter {};
                    }
                }, parameter.value),
                .name = parameter.name.identifier,
                .source_view = parameter.source_view
            };
        };

        if (parameters.vector) {
            return { bu::map(*parameters.vector, lower_template_parameter) };
        }
        else {
            return { std::nullopt };
        }
    }


    auto lower_function_parameter(Lowering_context& context) {
        return [&context](ast::Function_parameter const& parameter)
            -> hir::Function_parameter
        {
            return hir::Function_parameter {
                .pattern = context.lower(parameter.pattern),
                .type    = [&] {
                    if (parameter.type) {
                        return context.lower(*parameter.type);
                    }
                    else {
                        bu::always_assert(context.current_function_implicit_template_parameters != nullptr);

                        hir::Template_parameter type_parameter {
                            .value = hir::Template_parameter::Type_parameter {},
                            .name = context.fresh_upper_name()
                        };

                        hir::Type type {
                            .value = hir::type::Template_parameter_reference {
                                .name = type_parameter.name,
                                .explicit_parameter = false
                        }
                        };

                        context.current_function_implicit_template_parameters->push_back(
                            std::move(type_parameter)
                        );

                        return type;
                    }
                }(),
                .default_value = parameter.default_value.transform(context.lower())
            };
        };
    }


    struct Definition_lowering_visitor {
        Lowering_context     & context;
        ast::Definition const& this_definition;

        auto operator()(ast::definition::Function const& function) -> hir::Definition {
            std::vector<hir::Template_parameter> implicit_template_parameters;
            if (std::exchange(
                context.current_function_implicit_template_parameters,
                &implicit_template_parameters))
            {
                bu::abort("how did this happen");
            }

            // The parameters and the return type must be lowered first
            // in order to collect the implicit template parameters.
            auto parameters = bu::map(function.parameters, lower_function_parameter(context));
            auto return_type = function.return_type.transform(bu::compose(bu::wrap, context.lower()));

            context.current_function_implicit_template_parameters = nullptr;

            hir::definition::Function hir_function {
                .explicit_template_parameters = lower_template_parameters(
                    context,
                    function.template_parameters
                ),
                .implicit_template_parameters = hir::Template_parameters {
                    !implicit_template_parameters.empty()
                        ? std::optional { std::move(implicit_template_parameters) }
                        : std::nullopt
                },
                .parameters  = std::move(parameters),
                .return_type = std::move(return_type),
                .body        = context.lower(function.body),
                .name        = function.name
            };

            return {
                .value = std::move(hir_function),
                .source_view = this_definition.source_view
            };
        }

        auto operator()(auto const&) -> hir::Definition {
            bu::todo();
        }
    };

}


auto Lowering_context::lower(ast::Definition const& definition) -> hir::Definition {
    return std::visit(
        Definition_lowering_visitor {
            .context = *this,
            .this_definition = definition
        },
        definition.value
    );
}