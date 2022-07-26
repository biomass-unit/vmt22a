#include "bu/utilities.hpp"
#include "lowering_internals.hpp"


namespace {

    struct Definition_lowering_visitor {
        Lowering_context& context;

        auto operator()(ast::definition::Function const& function) -> hir::Definition::Variant {
            bu::always_assert(context.current_function_implicit_template_parameters == nullptr);
            std::vector<hir::Implicit_template_parameter> implicit_template_parameters;
            context.current_function_implicit_template_parameters = &implicit_template_parameters;

            // The parameters must be lowered first in order to collect the implicit template parameters.
            auto parameters = bu::map(context.lower())(function.parameters);

            context.current_function_implicit_template_parameters = nullptr;

            return hir::definition::Function {
                .explicit_template_parameters = context.lower(function.template_parameters),
                .implicit_template_parameters = std::move(implicit_template_parameters),
                .parameters                   = std::move(parameters),
                .return_type                  = function.return_type.transform(context.lower()),
                .body                         = context.lower(function.body),
                .name                         = function.name
            };
        }

        auto operator()(ast::definition::Struct const& structure) -> hir::Definition::Variant {
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

            return hir::definition::Struct {
                .members             = bu::map(lower_member)(structure.members),
                .name                = structure.name,
                .template_parameters = context.lower(structure.template_parameters),
            };
        }

        auto operator()(ast::definition::Enum const& enumeration) -> hir::Definition::Variant {
            auto const lower_constructor = [this](ast::definition::Enum::Constructor const& ctor)
                -> hir::definition::Enum::Constructor
            {
                return {
                    .name        = ctor.name,
                    .type        = ctor.type.transform(context.lower()),
                    .source_view = ctor.source_view
                };
            };

            return hir::definition::Enum {
                .constructors        = bu::map(lower_constructor)(enumeration.constructors),
                .name                = enumeration.name,
                .template_parameters = context.lower(enumeration.template_parameters)
            };
        }

        auto operator()(ast::definition::Alias const& alias) -> hir::Definition::Variant {
            return hir::definition::Alias {
                .name                = alias.name,
                .type                = context.lower(alias.type),
                .template_parameters = context.lower(alias.template_parameters)
            };
        }

        auto operator()(ast::definition::Typeclass const& typeclass) -> hir::Definition::Variant {
            return hir::definition::Typeclass {
                .function_signatures = bu::map(context.lower())(typeclass.function_signatures),
                .type_signatures     = bu::map(context.lower())(typeclass.type_signatures),
                .name                = typeclass.name,
                .template_parameters = context.lower(typeclass.template_parameters),
            };
        }

        auto operator()(ast::definition::Implementation const& implementation) -> hir::Definition::Variant {
            return hir::definition::Implementation {
                .type                = context.lower(implementation.type),
                .definitions         = bu::map(context.lower())(implementation.definitions),
                .template_parameters = context.lower(implementation.template_parameters),
            };
        }

        auto operator()(ast::definition::Instantiation const& instantiation) -> hir::Definition::Variant {
            return hir::definition::Instantiation {
                .typeclass           = context.lower(instantiation.typeclass),
                .instance            = context.lower(instantiation.instance),
                .definitions         = bu::map(context.lower())(instantiation.definitions),
                .template_parameters = context.lower(instantiation.template_parameters),
            };
        }

        auto operator()(ast::definition::Namespace const& space) -> hir::Definition::Variant {
            return hir::definition::Namespace {
                .definitions         = bu::map(context.lower())(space.definitions),
                .name                = space.name,
                .template_parameters = context.lower(space.template_parameters)
            };
        }
    };

}


auto Lowering_context::lower(ast::Definition const& definition) -> hir::Definition {
    bu::always_assert(!definition.value.valueless_by_exception());

    bu::Usize const old_kind = std::exchange(current_definition_kind, definition.value.index());
    auto hir_definition = std::visit(Definition_lowering_visitor { .context = *this }, definition.value);
    current_definition_kind = old_kind;

    return {
        .value = std::move(hir_definition),
        .source_view = definition.source_view
    };
}