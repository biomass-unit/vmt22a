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
                    [](ast::Template_parameter::Type_parameter const& type_parameter) {
                        return hir::Template_parameter::Type_parameter { type_parameter.classes };
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


    struct Definition_lowering_visitor {
        Lowering_context     & context;
        ast::Definition const& this_definition;

        auto operator()(ast::definition::Function const& function) -> hir::Definition {
            auto const lower_parameter = [this](ast::Function_parameter const& parameter) {
                return hir::Function_parameter {
                    .pattern       = context.lower(parameter.pattern),
                    .type          = parameter.type.transform(context.lower()),
                    .default_value = parameter.default_value.transform(context.lower())
                };
            };

            return {
                .value = hir::definition::Function {
                    .explicit_template_parameters = lower_template_parameters(
                        context,
                        function.template_parameters
                    ),
                    .implicit_template_parameters = { {} },
                    .parameters  = bu::map(function.parameters, lower_parameter),
                    .return_type = function.return_type
                                 . transform(bu::compose(bu::wrap, context.lower())),
                    .body = context.lower(function.body),
                    .name = function.name
                },
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