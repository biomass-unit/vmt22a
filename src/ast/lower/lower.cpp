#include "bu/utilities.hpp"
#include "lower.hpp"
#include "lowering_internals.hpp"


auto Lowering_context::lower(ast::Function_argument const& argument) -> hir::Function_argument {
    return { .expression = lower(argument.expression), .name = argument.name.transform(lower()) };
}

auto Lowering_context::lower(ast::Function_parameter const& parameter) -> hir::Function_parameter {
    return hir::Function_parameter {
        .pattern = lower(parameter.pattern),
        .type    = [this, &parameter] {
            if (parameter.type) {
                return lower(*parameter.type);
            }
            else {
                bu::always_assert(current_function_implicit_template_parameters != nullptr);

                hir::Template_parameter type_parameter {
                    .value = hir::Template_parameter::Type_parameter {},
                    .name  = fresh_upper_name()
                };

                hir::Type type {
                    .value = hir::type::Template_parameter_reference {
                        .name = type_parameter.name,
                        .explicit_parameter = false
                    }
                };

                current_function_implicit_template_parameters->push_back(
                    std::move(type_parameter)
                );

                return type;
            }
        }(),
        .default_value = parameter.default_value.transform(lower())
    };
}

auto Lowering_context::lower(ast::Template_argument const& argument) -> hir::Template_argument {
    return {
        .value = std::visit(bu::Overload {
            [](ast::Mutability const& mutability) -> hir::Template_argument::Variant {
                return mutability;
            },
            [this](auto const& argument) -> hir::Template_argument::Variant {
                return bu::wrap(lower(argument)); // Catches types and expressions
            }
        }, argument.value),
        .name = argument.name.transform(lower()),
    };
}

auto Lowering_context::lower(ast::Template_parameter const& parameter) -> hir::Template_parameter {
    return {
        .value = std::visit<hir::Template_parameter::Variant>(bu::Overload {
            [this](ast::Template_parameter::Type_parameter const& type_parameter) {
                return hir::Template_parameter::Type_parameter {
                    .classes = bu::map(lower())(type_parameter.classes)
                };
            },
            [this](ast::Template_parameter::Value_parameter const& value_parameter) {
                return hir::Template_parameter::Value_parameter {
                    .type = value_parameter.type.transform(bu::compose(bu::wrap, lower()))
                };
            },
            [](ast::Template_parameter::Mutability_parameter const&) {
                return hir::Template_parameter::Mutability_parameter {};
            }
        }, parameter.value),
        .name = lower(parameter.name),
        .source_view = parameter.source_view
    };
}

auto Lowering_context::lower(ast::Qualifier const& qualifier) -> hir::Qualifier {
    return {
        .template_arguments = qualifier.template_arguments.transform(bu::map(lower())),
        .name               = lower(qualifier.name),
        .source_view        = qualifier.source_view
    };
}

auto Lowering_context::lower(ast::Qualified_name const& name) -> hir::Qualified_name {
    return {
        .middle_qualifiers = bu::map(lower())(name.middle_qualifiers),
        .root_qualifier = std::visit(bu::Overload {
            [](std::monostate)                -> hir::Root_qualifier { return {}; },
            [](ast::Root_qualifier::Global)   -> hir::Root_qualifier { return { .value = hir::Root_qualifier::Global {} }; },
            [this](ast::Type const& type)     -> hir::Root_qualifier { return { .value = lower(type) }; }
        }, name.root_qualifier.value),
        .primary_name = lower(name.primary_name),
    };
}

auto Lowering_context::lower(ast::Class_reference const& reference) -> hir::Class_reference {
    return {
        .template_arguments = reference.template_arguments.transform(bu::map(lower())),
        .name = lower(reference.name),
        .source_view = reference.source_view,
    };
}

auto Lowering_context::lower(ast::Name const& name) -> hir::Name {
    return {
        .identifier  = name.identifier,
        .is_upper    = name.is_upper,
        .source_view = name.source_view
    };
}


auto ast::lower(Module&& module) -> hir::Module {
    hir::Module hir_module {
        .node_context {
            .wrapper_context {
                bu::Wrapper_context<hir::Expression> { module.node_context.arena_size<ast::Expression>() },
                bu::Wrapper_context<hir::Type>       { module.node_context.arena_size<ast::Type>() },
                bu::Wrapper_context<hir::Pattern>    { module.node_context.arena_size<ast::Pattern>() },
                bu::Wrapper_context<hir::Definition> { module.node_context.arena_size<ast::Definition>() }
            }
        },
        .diagnostics = std::move(module.diagnostics),
        .source = std::move(module.source)
    };

    Lowering_context context {
        hir_module.node_context,
        hir_module.diagnostics,
        hir_module.source
    };
    hir_module.definitions = bu::map(context.lower())(module.definitions);

    return hir_module;
}