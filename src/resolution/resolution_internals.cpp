#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


auto resolution::Scope::make_child() noexcept -> Scope {
    return { .current_frame_offset = current_frame_offset, .parent = this };
}

auto resolution::Scope::find(lexer::Identifier const name) noexcept -> Binding* {
    if (auto* const pointer = bindings.find(name)) {
        return pointer;
    }
    else {
        return parent ? parent->find(name) : nullptr;
    }
}

auto resolution::Scope::unused_variables() -> std::optional<std::vector<lexer::Identifier>> {
    std::vector<lexer::Identifier> names;

    for (auto& [name, binding] : bindings.container()) {
        if (!binding.has_been_mentioned) {
            names.push_back(name);
        }
    }

    if (names.empty()) {
        return std::nullopt;
    }
    else {
        return names;
    }
}


auto resolution::Resolution_context::resolve_mutability(ast::Mutability const mutability) -> bool {
    switch (mutability.type) {
    case ast::Mutability::Type::mut:
        return true;
    case ast::Mutability::Type::immut:
        return false;
    case ast::Mutability::Type::parameterized:
        if (!mutability_parameters) {
            return false;
        }
        else if (bool const* const parameter = mutability_parameters->find(*mutability.parameter_name)) {
            return *parameter;
        }
        else {
            auto const message = std::format(
                "{} is not a mutability parameter",
                *mutability.parameter_name
            );
            throw error({
                .erroneous_view = {},
                .message = message
            });
        }
    default:
        bu::unreachable();
    }
}

auto resolution::Resolution_context::apply_qualifiers(ast::Qualified_name& name) -> bu::Wrapper<Namespace> {
    bu::wrapper auto root = std::visit(bu::Overload {
        [&](std::monostate) {
            return current_namespace;
        },
        [&](ast::Root_qualifier::Global) {
            return global_namespace;
        },
        [](ast::Type&) -> bu::Wrapper<Namespace> {
            bu::unimplemented();
        }
    }, name.root_qualifier->value);

    for (auto& qualifier : name.middle_qualifiers) {
        if (qualifier.is_upper || qualifier.template_arguments) {
            bu::unimplemented();
        }

        if (bu::wrapper auto* const child = root->children.find(qualifier.name)) {
            root = *child;
        }
        else {
            bu::unimplemented();
        }
    }

    return root;
}


auto resolution::Resolution_context::make_child_context_with_new_scope() noexcept
    -> Resolution_context
{
    return {
        scope.make_child(),
        current_namespace,
        global_namespace,
        source,
        mutability_parameters,
        is_unevaluated
    };
}

auto resolution::Resolution_context::find_upper(ast::Qualified_name& name)
    -> std::optional<Upper_variant>
{
    assert(name.primary_qualifier.is_upper);
    return find_impl<&Namespace::upper_table, true>(name);
}

auto resolution::Resolution_context::find_lower(ast::Qualified_name& name)
    -> std::optional<Lower_variant>
{
    assert(!name.primary_qualifier.is_upper);
    if (name.is_unqualified()) {
        if (auto* binding = scope.find(name.primary_qualifier.name)) {
            return binding;
        }
    }
    return find_impl<&Namespace::lower_table, false>(name);
}

auto resolution::Resolution_context::error(Error_arguments const arguments)
    -> std::runtime_error
{
    return std::runtime_error {
        bu::textual_error({
            .erroneous_view = arguments.erroneous_view,
            .file_view      = source->string(),
            .file_name      = source->name(),
            .message        = arguments.message,
            .help_note      = arguments.help_note
        })
    };
}


auto resolution::resolve_template_arguments(ast::Qualified_name&               name,
                                            std::string_view                   name_source_view,
                                            std::span<ast::Template_parameter> parameters,
                                            std::span<ast::Template_argument>  arguments,
                                            Resolution_context&                context)
        -> ir::Template_argument_set
{
    auto const error = [&](std::string_view const message,
                           ast::Expression* const expression = nullptr)
        -> std::runtime_error
    {
        return context.error({
            .erroneous_view = expression ? expression->source_view : name_source_view,
            .message        = message
        });
    };

    ir::Template_argument_set argument_set;

    if (parameters.size() == arguments.size()) {
        for (bu::Usize i = 0; i != arguments.size(); ++i) {
            using Parameter = ast::Template_parameter;

            std::visit(bu::Overload {
                [&](Parameter::Type_parameter& parameter, ast::Type& type) {
                    // Check constraints here

                    argument_set.type_arguments.add(
                        bu::copy(parameter.name),
                        resolve_type(type, context)
                    );
                },
                [&](Parameter::Value_parameter& parameter, ast::Expression& expression) {
                    auto given_argument = resolve_expression(expression, context);
                    auto required_type  = resolve_type(parameter.type, context);

                    if (required_type == given_argument.type) {
                        argument_set.expression_arguments.add(
                            bu::copy(parameter.name),
                            std::move(given_argument)
                        );
                    }
                    else {
                        throw error(
                            std::format(
                                "The {} template argument should be of type "
                                "{}, but the given argument is of type {}",
                                bu::fmt::integer_with_ordinal_indicator(i + 1),
                                required_type,
                                given_argument.type
                            ),
                            &expression
                        );
                    }
                },
                [&](Parameter::Mutability_parameter& parameter, ast::Mutability const mutability) {
                    argument_set.mutability_arguments.add(
                        bu::copy(parameter.name),
                        context.resolve_mutability(mutability)
                    );
                },
                [](auto const&, auto const&) {
                    bu::abort("Template parameter-argument mismatch");
                }
            }, parameters[i].value, arguments[i].value);
        }

        return argument_set;
    }
    else {
        throw error(
            std::format(
                "{} expects {} template arguments, but {} were supplied",
                name,
                parameters.size(),
                arguments.size()
            )
        );
    }
}


auto ir::Type::is_unit() const noexcept -> bool {
    // Checking size == 0 isn't enough, because compound
    // types such as ((), ()) would also count as the unit type

    if (auto const* const tuple = std::get_if<ir::type::Tuple>(&value)) {
        return tuple->types.empty();
    }
    else {
        return false;
    }
}