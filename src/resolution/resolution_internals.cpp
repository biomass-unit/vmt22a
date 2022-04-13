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

auto resolution::Scope::find_type(lexer::Identifier const name) noexcept -> Local_type_alias* {
    if (auto* const pointer = local_type_aliases.find(name)) {
        return pointer;
    }
    else {
        return parent ? parent->find_type(name) : nullptr;
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


auto resolution::Namespace::format_name_as_member(lexer::Identifier const name) const -> std::string {
    std::string string;
    auto out = std::back_inserter(string);

    [out](this auto recurse, Namespace const* const space) -> void {
        if (space->parent) {
            recurse(std::to_address(*space->parent));
            if (space->name) {
                std::format_to(out, "{}::", space->name);
            }
        }
    }(this);

    std::format_to(out, "{}", name);

    return string;
}


auto resolution::Resolution_context::resolve_mutability(ast::Mutability const mutability) -> bool {
    switch (mutability.type) {
    case ast::Mutability::Type::mut:
        return true;
    case ast::Mutability::Type::immut:
        return false;
    case ast::Mutability::Type::parameterized:
        if (!mutability_arguments) {
            return false;
        }
        else {
            if (bool const* const argument = mutability_arguments->find(*mutability.parameter_name)) {
                return *argument;
            }
            else {
                throw error(
                    mutability.source_view,
                    std::format(
                        "{} is not a mutability parameter",
                        *mutability.parameter_name
                    )
                );
            }
        }
    default:
        bu::unreachable();
    }
}

auto resolution::Resolution_context::make_child_context_with_new_scope() noexcept
    -> Resolution_context
{
    return {
        scope.make_child(),
        current_namespace,
        global_namespace,
        source,
        mutability_arguments,
        is_unevaluated
    };
}

auto resolution::Resolution_context::error(
    bu::Source_view                 const source_view,
    std::string_view                const message,
    std::optional<std::string_view> const help
)
    -> std::runtime_error
{
    return std::runtime_error {
        bu::textual_error({
            .erroneous_view = source_view,
            .file_view      = source->string(),
            .file_name      = source->name(),
            .message        = message,
            .help_note      = help
        })
    };
}


namespace {

    template <class T>
    auto instantiate_template(
        lexer::Identifier                                         const name,
        bu::Source_view                                           const source_view,
        resolution::Definition<ast::definition::Template_definition<T>> template_definition,
        std::span<ast::Template_argument>                         const arguments,
        resolution::Resolution_context&                                 context
    )
        -> resolution::Definition<T>::Resolved_info
    {
        ir::Template_argument_set argument_set = resolution::resolve_template_arguments(
            name,
            source_view,
            template_definition.template_parameters,
            arguments,
            context
        );

        if (auto* const existing = template_definition.instantiations->find(argument_set)) {
            return *existing;
        }

        resolution::Resolution_context instantiation_context {
            .scope                { .parent = nullptr },
            .current_namespace    = template_definition.home_namespace,
            .global_namespace     = context.global_namespace,
            .source               = context.source,
            .mutability_arguments = &argument_set.mutability_arguments,
            .is_unevaluated       = false
        };

        if (argument_set.expression_arguments.size()) {
            bu::unimplemented();
        }

        for (auto& [parameter_name, type] : argument_set.type_arguments.container()) {

            // fix

            instantiation_context.scope.local_type_aliases.add(
                bu::copy(parameter_name),
                {
                    .type               = type,
                    .has_been_mentioned = false
                }
            );
        }

        resolution::Definition<T> concrete {
            .syntactic_definition = template_definition.syntactic_definition,
            .home_namespace       = template_definition.home_namespace,
            .resolved_info        = std::nullopt
        };

        // Resolve the instantiation as a regular non-template definition in the instantiation context
        instantiation_context.resolve_definition(concrete);

        if (!concrete.resolved_info->has_value()) {
            bu::abort("how");
        }

        bu::trivially_copyable auto instantiation_info = **concrete.resolved_info;

        // Add the template arguments to the name of the new concrete entity
        argument_set.append_formatted_arguments_to(instantiation_info.resolved->name);

        // Register the instantiation so that it can be retrieved later
        template_definition.instantiations->add(argument_set, bu::copy(instantiation_info));

        return instantiation_info;
    }


    auto find_type_impl(
        lexer::Identifier                                const name,
        bu::Source_view                                  const source_view,
        resolution::Upper_variant                        const upper,
        std::optional<std::span<ast::Template_argument>> const arguments,
        resolution::Resolution_context&                        context
    )
        -> bu::Wrapper<ir::Type>
    {
        return std::visit(bu::Overload {
            [&]<class T>(resolution::Definition<ast::definition::Template_definition<T>> definition)
                -> bu::Wrapper<ir::Type>
            {
                if (arguments) {
                    return instantiate_template(
                        name,
                        source_view,
                        definition,
                        *arguments,
                        context
                    ).type_handle;
                }
                else {
                    throw context.error(
                        source_view,
                        std::format(
                            "{} is a template, but no template arguments were provided",
                            name
                        )
                    );
                }
            },
            []<bu::one_of<ast::definition::Struct, ast::definition::Data, ast::definition::Alias> T>(
                resolution::Definition<T> definition
            )
                -> bu::Wrapper<ir::Type>
            {
                if (definition.has_been_resolved()) {
                    return (*definition.resolved_info)->type_handle;
                }
                else {
                    bu::unimplemented();
                }
            },
            [](auto&) -> bu::Wrapper<ir::Type> {
                bu::unimplemented();
            }
        }, upper);
    }

}


auto resolution::Resolution_context::find_type(
    ast::Qualified_name&                                   full_name,
    bu::Source_view                                  const source_view,
    std::optional<std::span<ast::Template_argument>> const arguments
)
    -> bu::Wrapper<ir::Type>
{
    assert(full_name.primary_name.is_upper);

    if (full_name.is_unqualified()) {
        if (Local_type_alias* const alias = scope.find_type(full_name.primary_name.identifier)) {
            alias->has_been_mentioned = true;
            return alias->type;
        }
    }

    return find_type_impl(
        full_name.primary_name.identifier,
        source_view,
        find_upper(full_name, source_view),
        arguments,
        *this
    );
}


auto resolution::Resolution_context::find_variable_or_function(
    ast::Qualified_name&                                   full_name,
    ast::Expression&                                       expression,
    std::optional<std::span<ast::Template_argument>> const arguments
)
    -> ir::Expression
{
    assert(!full_name.primary_name.is_upper);

    if (full_name.is_unqualified()) {
        // Try local lookup first

        if (Binding* const binding = scope.bindings.find(full_name.primary_name.identifier)) {
            binding->has_been_mentioned = true;

            if (!is_unevaluated && !binding->type->is_trivial) {
                if (binding->moved_by) {
                    bu::unimplemented();
                }
                else {
                    binding->moved_by = &expression;
                }
            }

            return {
                .value = ir::expression::Local_variable {
                    .frame_offset = binding->frame_offset
                },
                .type = binding->type
            };
        }
    }

    return std::visit(bu::Overload {
        [](Binding*) -> ir::Expression {
            bu::unimplemented(); // Unreachable?
        },
        [](bu::Wrapper<ir::definition::Data_constructor> constructor) -> ir::Expression {
            return {
                .value = ir::expression::Data_constructor_reference { constructor },
                .type  = constructor->function_type
            };
        },
        [&](Function_definition function) -> ir::Expression {
            if (arguments) {
                throw error(
                    expression.source_view,
                    std::format(
                        "{} is not a function template, but template arguments were provided",
                        full_name
                    )
                );
            }
            if (function.has_been_resolved()) {
                return {
                    .value = ir::expression::Function_reference { (*function.resolved_info)->resolved },
                    .type  = (*function.resolved_info)->type_handle
                };
            }
            else {
                bu::unimplemented();
            }
        },
        [&](Function_template_definition function_template) -> ir::Expression {
            if (!arguments) {
                throw error(
                    expression.source_view,
                    std::format(
                        "{} is a function template, but no template arguments were provided",
                        full_name
                    )
                );
            }

            auto info = instantiate_template(
                full_name.primary_name.identifier,
                expression.source_view,
                function_template,
                *arguments,
                *this
            );

            return {
                .value = ir::expression::Function_reference { info.resolved },
                .type  = info.type_handle
            };
        }
    }, find_lower(full_name, expression.source_view));
}


auto resolution::Namespace::find_type_here(
    lexer::Identifier                                const name,
    bu::Source_view                                  const source_view,
    std::optional<std::span<ast::Template_argument>> const arguments,
    Resolution_context&                                    context
)
    -> std::optional<bu::Wrapper<ir::Type>>
{
    if (auto* const upper = upper_table.find(name)) {
        return find_type_impl(
            name,
            source_view,
            *upper,
            arguments,
            context
        );
    }
    else {
        return std::nullopt;
    }
}


auto resolution::resolve_template_arguments(
    lexer::Identifier                  const name,
    bu::Source_view                    const source_view,
    std::span<ast::Template_parameter> const parameters,
    std::span<ast::Template_argument>  const arguments,
    Resolution_context&                      context
)
    -> ir::Template_argument_set
{
    ir::Template_argument_set argument_set;
    argument_set.arguments_in_order.reserve(parameters.size());

    if (parameters.size() == arguments.size()) {
        for (bu::Usize i = 0; i != arguments.size(); ++i) {
            using Parameter = ast::Template_parameter;

            std::visit(bu::Overload {
                [&](Parameter::Type_parameter& parameter, ast::Type& type) {
                    // Check constraints here

                    argument_set.arguments_in_order.emplace_back(
                        ir::Template_argument_set::Argument_indicator::Kind::type,
                        argument_set.type_arguments.size()
                    );
                    argument_set.type_arguments.add(
                        bu::copy(parameter.name),
                        context.resolve_type(type)
                    );
                },
                [&](Parameter::Value_parameter& parameter, ast::Expression& expression) {
                    auto given_argument = context.resolve_expression(expression);
                    auto required_type  = context.resolve_type(parameter.type);

                    if (required_type == given_argument.type) {
                        argument_set.arguments_in_order.emplace_back(
                            ir::Template_argument_set::Argument_indicator::Kind::expression,
                            argument_set.expression_arguments.size()
                        );
                        argument_set.expression_arguments.add(
                            bu::copy(parameter.name),
                            std::move(given_argument)
                        );
                    }
                    else {
                        throw context.error(
                            expression.source_view,
                            std::format(
                                "The {} template argument should be of type "
                                "{}, but the given argument is of type {}",
                                bu::fmt::integer_with_ordinal_indicator(i + 1),
                                required_type,
                                given_argument.type
                            )
                        );
                    }
                },
                [&](Parameter::Mutability_parameter& parameter, ast::Mutability const mutability) {
                    argument_set.arguments_in_order.emplace_back(
                        ir::Template_argument_set::Argument_indicator::Kind::mutability,
                        argument_set.mutability_arguments.size()
                    );
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
        throw context.error(
            source_view,
            std::format(
                "{} expects {} template arguments, but {} were supplied",
                context.current_namespace->format_name_as_member(name),
                parameters.size(),
                arguments.size()
            )
        );
    }
}


auto resolution::definition_description(Lower_variant const variant)
    noexcept -> std::string_view
{
    return std::visit(bu::Overload {
        [](Binding                          const*) { return "a let-binding"; },
        [](ir::definition::Data_constructor const&) { return "a data constructor"; },
        [](Function_definition              const&) { return "a function definition"; },
        [](Function_template_definition     const&) { return "a function template definition"; }
    }, variant);
}

auto resolution::definition_description(Upper_variant const variant)
    noexcept -> std::string_view
{
    return std::visit(bu::Overload {
        [](Struct_definition             const&) { return "a struct definition"; },
        [](Struct_template_definition    const&) { return "a struct template definition"; },
        [](Data_definition               const&) { return "a data definition"; },
        [](Data_template_definition      const&) { return "a data template definition"; },
        [](Alias_definition              const&) { return "an alias definition"; },
        [](Alias_template_definition     const&) { return "an alias template definition"; },
        [](Typeclass_definition          const&) { return "a class definition"; },
        [](Typeclass_template_definition const&) { return "a class template definition"; },
    }, variant);
}


auto ir::Template_argument_set::append_formatted_arguments_to(std::string& string) const -> void {
    string.push_back('[');

    auto out = std::back_inserter(string);

    for (auto [kind, index] : arguments_in_order) {
        switch (kind) {
        case Argument_indicator::Kind::type:
            std::format_to(out, "{}, ", type_arguments.container().at(index).second);
            break;
        case Argument_indicator::Kind::expression:
            std::format_to(out, "{}, ", expression_arguments.container().at(index).second);
            break;
        case Argument_indicator::Kind::mutability:
            std::format_to(out, "{}, ", mutability_arguments.container().at(index).second ? "mut" : "immut");
            break;
        default:
            bu::unreachable();
        }
    }

    if (string.ends_with(", ")) {
        string.erase(string.end() - 2, string.end());
    }

    string.push_back(']');

    // Improve later
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