#include "bu/utilities.hpp"
#include "lowering_internals.hpp"


namespace {

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
            auto parameters = bu::map(context.lower())(function.parameters);
            auto return_type = function.return_type.transform(bu::compose(bu::wrap, context.lower()));

            context.current_function_implicit_template_parameters = nullptr;

            hir::definition::Function hir_function {
                .explicit_template_parameters = function.template_parameters.vector.transform(bu::map(context.lower())),
                .implicit_template_parameters = !implicit_template_parameters.empty()
                                              ? std::optional { std::move(implicit_template_parameters) }
                                              : std::nullopt,
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