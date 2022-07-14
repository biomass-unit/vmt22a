#include "bu/utilities.hpp"
#include "lowering_internals.hpp"


namespace {

    struct Definition_lowering_visitor {
        Lowering_context     & context;
        ast::Definition const& this_definition;

        auto operator()(ast::definition::Function const& function) -> hir::Definition {
            bool const old_is_within_function = std::exchange(context.is_within_function, true);

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
                .name        = context.lower(function.name)
            };

            context.is_within_function = old_is_within_function;

            return {
                .value = std::move(hir_function),
                .source_view = this_definition.source_view
            };
        }

        auto operator()(ast::definition::Struct const& structure) -> hir::Definition {
            auto const lower_member = [this](ast::definition::Struct::Member const& member)
                -> hir::definition::Struct::Member
            {
                return {
                    .name        = member.name,
                    .type        = context.lower(member.type),
                    .is_public   = member.is_public,
                    .source_view = member.source_view
                };
            };

            return {
                .value = hir::definition::Struct {
                    .template_parameters = structure.template_parameters.vector.transform(bu::map(context.lower())),
                    .members = bu::map(lower_member)(structure.members),
                    .name = context.lower(structure.name)
                },
                .source_view = this_definition.source_view
            };
        }

        auto operator()(ast::definition::Enum const& enumeration) -> hir::Definition {
            auto const lower_constructor = [this](ast::definition::Enum::Constructor const& ctor)
                -> hir::definition::Enum::Constructor
            {
                return {
                    .name = ctor.name,
                    .type = ctor.type.transform(context.lower()),
                    .source_view = ctor.source_view
                };
            };

            return {
                .value = hir::definition::Enum {
                    .constructors = bu::map(lower_constructor)(enumeration.constructors),
                    .name = context.lower(enumeration.name),
                    .template_parameters = enumeration.template_parameters.vector.transform(bu::map(context.lower()))
                },
                .source_view = this_definition.source_view
            };
        }

        auto operator()(ast::definition::Alias const&) -> hir::Definition {
            bu::todo();
        }

        auto operator()(ast::definition::Namespace const&) -> hir::Definition {
            bu::todo();
        }

        auto operator()(ast::definition::Typeclass const&) -> hir::Definition {
            bu::todo();
        }

        auto operator()(ast::definition::Implementation const&) -> hir::Definition {
            bu::todo();
        }

        auto operator()(ast::definition::Instantiation const&) -> hir::Definition {
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