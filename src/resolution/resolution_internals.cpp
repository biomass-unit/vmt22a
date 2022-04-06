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


auto resolution::Namespace::find_root(ast::Qualifier& qualifier) -> bu::Wrapper<Namespace> {
    if (qualifier.is_upper || qualifier.template_arguments) {
        bu::unimplemented();
    }

    static constexpr auto to_pointer = [](std::optional<bu::Wrapper<Namespace>> space)
        noexcept -> Namespace*
    {
        return space ? &**space : nullptr;
    };

    for (Namespace* space = this; space; space = to_pointer(space->parent)) {
        if (bu::wrapper auto* child = space->children.find(qualifier.name)) {
            return *child;
        }
    }

    bu::abort("couldn't find root namespace");
}

auto resolution::Namespace::format_name_as_member(lexer::Identifier const name) const -> std::string {
    std::string string;
    auto out = std::back_inserter(string);

    [out](this auto recurse, Namespace const* const space) -> void {
        if (space->parent) {
            recurse(std::to_address(*space->parent));
            std::format_to(out, "{}::", space->name);
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
                auto const message = std::format(
                    "{} is not a mutability parameter",
                    *mutability.parameter_name
                );
                throw error({
                    .erroneous_view = mutability.source_view,
                    .message        = message
                });
            }
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
            bu::unimplemented(); // ir::definition::User_defined_*::associated_namespace
        }
    }, name.root_qualifier->value);

    auto const ensure_regular_qualifier = [](ast::Qualifier& qualifier) -> void {
        if (qualifier.is_upper || qualifier.template_arguments.has_value()) {
            bu::unimplemented();
        }
    };

    if (!name.middle_qualifiers.empty()) {
        root = root->find_root(name.middle_qualifiers.front());

        for (auto& qualifier : name.middle_qualifiers | std::views::drop(1)) {
            ensure_regular_qualifier(qualifier);

            if (bu::wrapper auto* const child = root->children.find(qualifier.name)) {
                root = *child;
            }
            else {
                bu::unimplemented();
            }
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
        mutability_arguments,
        is_unevaluated
    };
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


auto resolution::Resolution_context::find_type(ast::Qualified_name&   full_name,
                                               std::string_view const source_view)
    -> bu::Wrapper<ir::Type>
{
    assert(full_name.primary_qualifier.is_upper);
    lexer::Identifier const name = full_name.primary_qualifier.name;

    if (full_name.is_unqualified()) {
        if (Local_type_alias* const alias = scope.find_type(name)) {
            alias->has_been_mentioned = true;
            return alias->type;
        }
    }

    bu::wrapper auto space = apply_qualifiers(full_name);

    if (auto* const upper = space->upper_table.find(name)) {
        return std::visit(bu::Overload {
            [](Struct_definition structure) -> bu::Wrapper<ir::Type> {
                if (structure.has_been_resolved()) {
                    return (*structure.resolved_info)->type_handle;
                }
                else {
                    bu::unimplemented();
                }
            },
            [](Data_definition)      -> bu::Wrapper<ir::Type> { bu::unimplemented(); },
            [](Alias_definition)     -> bu::Wrapper<ir::Type> { bu::unimplemented(); },
            [](Typeclass_definition) -> bu::Wrapper<ir::Type> { bu::unimplemented(); },
            []<class T>(Definition<ast::definition::Template_definition<T>>)
                -> bu::Wrapper<ir::Type>
            {
                bu::abort("was expecting a type but found a template");
            }
        }, *upper);
    }
    else {
        auto const message = std::format("{} is undefined", full_name);
        throw error({ .erroneous_view = source_view, .message = message });
    }
}

auto resolution::Resolution_context::find_type_template_instantiation(ast::Qualified_name&                    full_name,
                                                                      std::string_view const                  source_view,
                                                                      std::span<ast::Template_argument> const arguments)
    -> bu::Wrapper<ir::Type>
{
    assert(full_name.primary_qualifier.is_upper);
    bu::wrapper auto space = apply_qualifiers(full_name);

    if (auto* const upper = space->upper_table.find(full_name.primary_qualifier.name)) {
        return std::visit(bu::Overload {
            [&]<class T>(Definition<ast::definition::Template_definition<T>> template_definition)
                -> bu::Wrapper<ir::Type>
            {
                ir::Template_argument_set argument_set = resolve_template_arguments(
                    full_name,
                    source_view,
                    template_definition.template_parameters,
                    arguments,
                    *this
                );

                if (auto* const existing = template_definition.instantiations->find(argument_set)) {
                    return existing->type_handle;
                }

                Resolution_context instantiation_context {
                    .scope                { .parent = nullptr },
                    .current_namespace    = template_definition.home_namespace,
                    .global_namespace     = global_namespace,
                    .source               = source,
                    .mutability_arguments = &argument_set.mutability_arguments,
                    .is_unevaluated       = false
                };

                if (argument_set.expression_arguments.size() || argument_set.mutability_arguments.size()) {
                    bu::unimplemented();
                }

                for (auto& [name, type] : argument_set.type_arguments.container()) {
                    instantiation_context.scope.local_type_aliases.add(
                        bu::copy(name),
                        {
                            .type               = type,
                            .has_been_mentioned = false
                        }
                    );
                }

                Definition<T> concrete {
                    .syntactic_definition = template_definition.syntactic_definition,
                    .home_namespace       = template_definition.home_namespace,
                    .resolved_info        = std::nullopt
                };

                // Resolve the instantiation as a regular non-template definition in the instantiation context
                resolve_definition(concrete, instantiation_context);

                if (!concrete.resolved_info->has_value()) {
                    bu::abort("how");
                }

                bu::trivially_copyable auto instantiation_info = **concrete.resolved_info;

                // Add the template arguments to the name
                argument_set.append_formatted_arguments(instantiation_info.resolved->name);

                // Register the instantiation so that it can be retrieved later
                template_definition.instantiations->add(argument_set, bu::copy(instantiation_info));

                // Return a handle to the newly instantiated type
                return instantiation_info.type_handle;
            },
            [](auto&) -> bu::Wrapper<ir::Type> {
                bu::unimplemented();
            }
        }, *upper);
    }
    else {
        auto const message = std::format("{} is undefined", full_name);
        throw error({ .erroneous_view = source_view, .message = message });
    }
}

auto resolution::Resolution_context::find_variable_or_function(ast::Qualified_name&   full_name,
                                                               std::string_view const source_view)
    -> Lower_variant
{
    assert(!full_name.primary_qualifier.is_upper);
    lexer::Identifier const name = full_name.primary_qualifier.name;

    if (full_name.is_unqualified()) {
        if (Binding* const binding = scope.find(name)) {
            return binding;
        }
    }

    bu::wrapper auto space = apply_qualifiers(full_name);

    if (Lower_variant* const variant = space->lower_table.find(name)) {
        return *variant;
    }
    else {
        auto const message = std::format("{} is undefined", full_name);
        throw error({ .erroneous_view = source_view, .message = message });
    }
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

                    argument_set.arguments_in_order.emplace_back(
                        ir::Template_argument_set::Argument_indicator::Kind::type,
                        argument_set.type_arguments.size()
                    );
                    argument_set.type_arguments.add(
                        bu::copy(parameter.name),
                        resolve_type(type, context)
                    );
                },
                [&](Parameter::Value_parameter& parameter, ast::Expression& expression) {
                    auto given_argument = resolve_expression(expression, context);
                    auto required_type  = resolve_type(parameter.type, context);

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


auto ir::Template_argument_set::append_formatted_arguments(std::string& string) const -> void {
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