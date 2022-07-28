#include "bu/utilities.hpp"
#include "lower.hpp"
#include "lowering_internals.hpp"
#include "mir/mir.hpp"


Lowering_context::Lowering_context(
    hir::Node_context       & node_context,
    bu::diagnostics::Builder& diagnostics,
    bu::Source         const& source
) noexcept
    : node_context { node_context }
    , diagnostics  { diagnostics }
    , source       { source } {}


auto Lowering_context::is_within_function() const noexcept -> bool {
    return current_definition_kind
        == bu::alternative_index<ast::Definition::Variant, ast::definition::Function>;
}

auto Lowering_context::fresh_name_tag() -> bu::Usize {
    return current_name_tag++.get();
}


auto Lowering_context::lower(ast::Function_argument const& argument) -> hir::Function_argument {
    return { .expression = lower(argument.expression), .name = argument.name };
}

auto Lowering_context::lower(ast::Function_parameter const& parameter) -> hir::Function_parameter {
    return hir::Function_parameter {
        .pattern = lower(parameter.pattern),
        .type    = [this, &parameter]() -> hir::Type {
            if (parameter.type) {
                return lower(*parameter.type);
            }
            else {
                bu::always_assert(current_function_implicit_template_parameters != nullptr);
                auto const tag = fresh_name_tag();
                current_function_implicit_template_parameters->push_back({ .tag = tag });
                return {
                    .value       = hir::type::Implicit_parameter_reference { .tag = tag },
                    .source_view = parameter.pattern.source_view
                };
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
            [this](ast::Type const& type) -> hir::Template_argument::Variant {
                return bu::wrap(lower(type));
            },
            [this](ast::Expression const& expression) -> hir::Template_argument::Variant {
                error(expression.source_view, { "Constant evaluation is not supported yet" });
            }
        }, argument.value),
        .name = argument.name,
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
        .name        = parameter.name,
        .source_view = parameter.source_view
    };
}

auto Lowering_context::lower(ast::Template_parameters const& parameters) -> hir::Template_parameters {
    return parameters.vector.transform(bu::map(lower()));
}

auto Lowering_context::lower(ast::Qualifier const& qualifier) -> hir::Qualifier {
    return {
        .template_arguments = qualifier.template_arguments.transform(bu::map(lower())),
        .name               = qualifier.name,
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
        .primary_name = name.primary_name,
    };
}

auto Lowering_context::lower(ast::Class_reference const& reference) -> hir::Class_reference {
    return {
        .template_arguments = reference.template_arguments.transform(bu::map(lower())),
        .name = lower(reference.name),
        .source_view = reference.source_view,
    };
}

auto Lowering_context::lower(ast::Function_signature const& signature) -> hir::Function_signature {
    return {
        .template_parameters = lower(signature.template_parameters),
        .argument_types      = bu::map(lower())(signature.argument_types),
        .return_type         = lower(signature.return_type),
        .name                = signature.name,
    };
}

auto Lowering_context::lower(ast::Type_signature const& signature) -> hir::Type_signature {
    return {
        .template_parameters = lower(signature.template_parameters),
        .classes             = bu::map(lower())(signature.classes),
        .name                = signature.name,
    };
}


auto Lowering_context::unit_value(bu::Source_view const view) -> bu::Wrapper<hir::Expression> {
    return hir::Expression { .value = hir::expression::Tuple {}, .source_view = view };
}
auto Lowering_context::wildcard_pattern(bu::Source_view const view) -> bu::Wrapper<hir::Pattern> {
    return hir::Pattern { .value = hir::pattern::Wildcard {}, .source_view = view};
}
auto Lowering_context::true_pattern(bu::Source_view const view) -> bu::Wrapper<hir::Pattern> {
    return hir::Pattern { .value = hir::pattern::Literal<bool> { true }, .source_view = view };
}
auto Lowering_context::false_pattern(bu::Source_view const view) -> bu::Wrapper<hir::Pattern> {
    return hir::Pattern { .value = hir::pattern::Literal<bool> { false }, .source_view = view };
}


auto Lowering_context::error(
    bu::Source_view                    const erroneous_view,
    bu::diagnostics::Message_arguments const arguments) const -> void
{
    diagnostics.emit_simple_error(arguments.add_source_info(source, erroneous_view));
}


auto ast::lower(Module&& module) -> hir::Module {
    hir::Module hir_module {
        .node_context {
            bu::Wrapper_context<hir::Expression> { module.node_context.arena_size<ast::Expression>() },
            bu::Wrapper_context<hir::Type>       { module.node_context.arena_size<ast::Type>() },
            bu::Wrapper_context<hir::Pattern>    { module.node_context.arena_size<ast::Pattern>() },
        },
        .diagnostics = std::move(module.diagnostics),
        .source      = std::move(module.source)
    };

    Lowering_context context {
        hir_module.node_context,
        hir_module.diagnostics,
        hir_module.source
    };
    hir_module.definitions = bu::map(context.lower())(module.definitions);

    return hir_module;
}