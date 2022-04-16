#include "bu/utilities.hpp"
#include "resolution.hpp"
#include "resolution_internals.hpp"

#include "lexer/lexer.hpp"
#include "parser/parser.hpp"


namespace {

    auto handle_imports(ast::Module& module) -> void {
        auto project_root = std::filesystem::current_path() / "sample-project"; // for debugging purposes, programmatically find path later

        std::vector<ast::Module> modules;
        modules.reserve(module.imports.size());

        for (auto& import : module.imports) {
            auto directory = project_root;

            for (auto& component : import.path | std::views::take(import.path.size() - 1)) {
                directory /= component.view();
            }

            auto path = directory / (std::string { import.path.back().view() } + ".vmt"); // fix later
            modules.push_back(parser::parse(lexer::lex(bu::Source { path.string() })));
        }

        {
            // add stuff to the namespace
        }
    }


    auto ensure_no_clashing_definitions(
        resolution::Namespace const& space,
        ast::Name             const& name,
        bu::Source*           const  source
    )
        -> void
    {
        auto const error = [&](bu::Pair<std::string_view, bu::Source_view> description)
            noexcept -> resolution::Resolution_context::Error
        {
            auto const sections = std::to_array({
                bu::Highlighted_text_section {
                    .source_view = description.second,
                    .source      = source,
                    .note        = "Originally defined here",
                    .note_color  = bu::text_warning_color
                },
                bu::Highlighted_text_section {
                    .source_view = name.source_view,
                    .source      = source,
                    .note        = "Later redefined here",
                    .note_color  = bu::text_error_color
                }
            });

            auto const message = std::format(
                "This namespace already contains {} with name {}",
                description.first,
                name
            );

            return resolution::Resolution_context::Error {
                bu::textual_error({
                    .sections = sections,
                    .source   = source,
                    .message  = message,
                })
            };
        };

        if (name.is_upper) {
            if (auto* const variant = space.upper_table.find(name.identifier)) {
                throw error(resolution::definition_description(*variant));
            }
        }
        else {
            if (auto* const variant = space.lower_table.find(name.identifier)) {
                throw error(resolution::definition_description(*variant));
            }
            else if (auto* const child = space.children.find(name.identifier)) {
                if ((*child)->name.has_value()) {
                    throw error({ "a namespace definition", (*child)->name->source_view });
                }
                else {
                    bu::unimplemented();
                }
            }
        }
    }


    auto make_namespace(
        std::optional<bu::Wrapper<resolution::Namespace>> parent,
        std::optional<ast::Name>                    const name,
        bu::Source*                                 const source,
        std::span<ast::Definition>                  const definitions
    )
        -> bu::Wrapper<resolution::Namespace>
    {
        bu::Wrapper space {
            resolution::Namespace {
                .parent = parent,
                .name   = name
            }
        };

        for (auto& definition : definitions) {
            using namespace ast::definition;

            auto const ensure_no_clashes = [&](ast::Name const& name) {
                ensure_no_clashing_definitions(space, name, source);
            };

            std::visit(bu::Overload {
                [&](Function& function) -> void {
                    resolution::Function_definition handle {
                        .syntactic_definition = &function,
                        .home_namespace       = space
                    };
                    ensure_no_clashes(function.name);
                    space->definitions_in_order.push_back(handle);
                    space->lower_table.add(bu::copy(function.name.identifier), bu::copy(handle));
                },
                [&]<bu::one_of<Struct, Data, Alias, Typeclass> T>(T& definition) -> void {
                    resolution::Definition<T> handle {
                        .syntactic_definition = &definition,
                        .home_namespace       = space
                    };
                    ensure_no_clashes(definition.name);
                    space->definitions_in_order.push_back(handle);
                    space->upper_table.add(bu::copy(definition.name.identifier), bu::copy(handle));
                },
                [&]<bu::one_of<Struct, Data, Alias, Typeclass> T>(Template_definition<T>& template_definition) -> void {
                    resolution::Definition<Template_definition<T>> handle {
                        .syntactic_definition = &template_definition.definition,
                        .home_namespace       = space,
                        .template_parameters  = template_definition.parameters
                    };
                    ensure_no_clashes(template_definition.definition.name);
                    space->definitions_in_order.push_back(handle);
                    space->upper_table.add(bu::copy(template_definition.definition.name.identifier), bu::copy(handle));
                },
                [&](Function_template& function_template) -> void {
                    resolution::Function_template_definition handle {
                        .syntactic_definition = &function_template.definition,
                        .home_namespace       = space,
                        .template_parameters  = function_template.parameters
                    };
                    ensure_no_clashes(function_template.definition.name);
                    space->definitions_in_order.push_back(handle);
                    space->lower_table.add(bu::copy(function_template.definition.name.identifier), bu::copy(handle));
                },

                [&](Implementation& implementation) -> void {
                    resolution::Implementation_definition handle {
                        .syntactic_definition = &implementation,
                        .home_namespace = make_namespace(
                            std::nullopt,
                            std::nullopt,
                            source,
                            implementation.definitions
                        )
                    };
                    space->definitions_in_order.push_back(handle);
                },
                [](Instantiation&) -> void {
                    bu::unimplemented();
                },

                [&](Namespace& nested_space) -> void {
                    bu::wrapper auto child = make_namespace(
                        space,
                        nested_space.name,
                        source,
                        nested_space.definitions
                    );
                    space->definitions_in_order.push_back(child);
                    space->children.add(bu::copy(nested_space.name.identifier), bu::copy(child));
                },

                [](auto&) -> void {
                    bu::unimplemented();
                }
            }, definition.value);
        }

        return space;
    }


    struct Definition_resolution_visitor {
        resolution::Resolution_context& context;


        auto make_associated_namespace(ast::Name const& type_name) const
            -> bu::Wrapper<resolution::Namespace>
        {
            assert(type_name.is_upper);
            return resolution::Namespace {
                .parent = context.current_namespace,
                .name   = type_name
            };
        }


        auto operator()(resolution::Function_definition function) -> void {
            if (function.has_been_resolved()) {
                return;
            }

            // Shadow the struct member, prevent polluting the global scope
            auto context = this->context.make_child_context_with_new_scope();

            auto* const definition = function.syntactic_definition;

            std::vector<ir::definition::Function::Parameter> parameters;
            parameters.reserve(definition->parameters.size());

            for (auto& [pattern, type, default_value] : definition->parameters) {
                bu::wrapper auto ir_type = context.resolve_type(type);
                context.bind(pattern, ir_type);

                parameters.emplace_back(
                    ir_type,
                    default_value.transform(
                        bu::compose(bu::wrap, resolution::resolve_expression_with(context))
                    )
                );
            }

            bu::Wrapper body = context.resolve_expression(definition->body);

            if (definition->return_type) {
                bu::wrapper auto explicit_type = context.resolve_type(*definition->return_type);

                if (body->type != explicit_type) {
                    auto const type_requirement_message = std::format(
                        "Expected {}, but found {}",
                        explicit_type,
                        body->type
                    );

                    auto const sections = std::to_array({
                        bu::Highlighted_text_section {
                            .source_view = (*definition->return_type)->source_view,
                            .source      = context.source,
                            .note        = "Return type explicitly specified here",
                            .note_color  = bu::text_warning_color
                        },
                        bu::Highlighted_text_section {
                            .source_view = definition->body.source_view,
                            .source      = context.source,
                            .note        = type_requirement_message,
                            .note_color  = bu::text_error_color
                        }
                    });

                    throw resolution::Resolution_context::Error {
                        bu::textual_error({
                            .sections = sections,
                            .source   = context.source,
                            .message  = "Mismatched types"
                        })
                    };
                }
            }

            std::vector<bu::Wrapper<ir::Type>> parameter_types;
            parameter_types.reserve(parameters.size());

            std::ranges::copy(
                parameters | std::views::transform(&ir::definition::Function::Parameter::type),
                std::back_inserter(parameter_types)
            );

            *function.resolved_info = resolution::Function_definition::Resolved_info {
                .resolved = ir::definition::Function {
                    .name        = context.current_namespace->format_name_as_member(definition->name.identifier),
                    .parameters  = std::move(parameters),
                    .return_type = body->type,
                    .body        = body
                },
                .type_handle = ir::Type {
                    .value      = ir::type::Function { std::move(parameter_types), body->type },
                    .size       = ir::Size_type { bu::unchecked_tag, 2 * sizeof(std::byte*) },
                    .is_trivial = true // for now
                }
            };
        }

        auto operator()(resolution::Struct_definition structure) -> void {
            if (structure.has_been_resolved()) {
                return;
            }

            auto* const definition = structure.syntactic_definition;

            bu::Flatmap<lexer::Identifier, ir::definition::Struct::Member> members;
            members.container().reserve(definition->members.size());

            ir::Size_type size;
            bool is_trivial = true;

            for (auto& member : definition->members) {
                bu::wrapper auto    type   = context.resolve_type(member.type);
                ir::Size_type const offset = size;

                if (!type->is_trivial) {
                    is_trivial = false;
                }
                size.safe_add(type->size);

                members.add(
                    bu::copy(member.name),
                    {
                        .type      = type,
                        .offset    = offset.safe_cast<bu::U16>(),
                        .is_public = member.is_public
                    }
                );
            }

            bu::Wrapper resolved_structure = ir::definition::Struct {
                .members              = std::move(members),
                .name                 = context.current_namespace->format_name_as_member(definition->name.identifier),
                .associated_namespace = make_associated_namespace(definition->name),
                .size                 = size,
                .is_trivial           = is_trivial
            };

            *structure.resolved_info = resolution::Struct_definition::Resolved_info {
                .resolved = resolved_structure,
                .type_handle = ir::Type {
                    .value      = ir::type::User_defined_struct { resolved_structure },
                    .size       = resolved_structure->size,
                    .is_trivial = resolved_structure->is_trivial
                }
            };
        }

        auto operator()(resolution::Data_definition data) -> void {
            if (data.has_been_resolved()) {
                return;
            }

            auto* const definition = data.syntactic_definition;

            bu::Flatmap<ast::Name, bu::Pair<std::optional<bu::Wrapper<ir::Type>>, bu::U8>> constructors;
            constructors.container().reserve(definition->constructors.size());

            ir::Size_type size;
            bool          is_trivial = true;
            bu::U8        tag        = 0;
            // The tag doesn't have to be a bu::bounded_u8 because the
            // parser disallows data-definitions with too many constructors

            for (auto& constructor : definition->constructors) {
                auto type = constructor.type.transform(resolution::resolve_type_with(context));

                if (type) {
                    if (!(*type)->is_trivial) {
                        is_trivial = false;
                    }
                    size = ir::Size_type { std::max(size.get(), (*type)->size.get()) };
                }

                constructors.add(
                    {
                        .identifier  = constructor.name,
                        .is_upper    = false,
                        .source_view = constructor.source_view
                    },
                    { type, tag++ }
                );
            }


            bu::Wrapper resolved_data = ir::definition::Data {
                .name                 = context.current_namespace->format_name_as_member(definition->name.identifier),
                .associated_namespace = make_associated_namespace(definition->name),
                .size                 = size,
                .is_trivial           = is_trivial
            };

            bu::Wrapper type_handle = ir::Type {
                .value      = ir::type::User_defined_data { resolved_data },
                .size       = size,
                .is_trivial = is_trivial
            };


            bu::Flatmap<
                std::optional<bu::Wrapper<ir::Type>>,
                bu::Wrapper<ir::Type>
            > function_types;

            for (auto& [name, constructor] : constructors.container()) {
                auto& [type, tag] = constructor;

                bu::wrapper auto function_type = [&] {
                    if (bu::wrapper auto* const existing = function_types.find(type)) {
                        return *existing;
                    }
                    else {
                        std::vector<bu::Wrapper<ir::Type>> parameter_types;
                        if (type) {
                            parameter_types.push_back(*type);
                        }

                        bu::Wrapper function_type = ir::Type {
                            .value = ir::type::Function {
                                .parameter_types = std::move(parameter_types),
                                .return_type     = type_handle
                            },
                            .size       = ir::Size_type { bu::unchecked_tag, 2 * sizeof(std::byte*) },
                            .is_trivial = true
                        };

                        function_types.add(bu::copy(type), bu::copy(function_type));

                        return function_type;
                    }
                }();

                resolved_data->associated_namespace->lower_table.add(
                    bu::copy(name.identifier),
                    ir::definition::Data_constructor {
                        .payload_type  = type,
                        .function_type = function_type,
                        .data_type     = type_handle,
                        .name          = name.identifier,
                        .tag           = tag,
                        .source_view   = name.source_view
                    }
                );
            }


            *data.resolved_info = resolution::Data_definition::Resolved_info {
                .resolved    = resolved_data,
                .type_handle = type_handle
            };
        }

        auto operator()(resolution::Alias_definition alias) -> void {
            if (alias.has_been_resolved()) {
                return;
            }

            auto* const definition = alias.syntactic_definition;

            bu::Wrapper resolved_alias = ir::definition::Alias {
                .name = context.current_namespace->format_name_as_member(definition->name.identifier),
                .type = context.resolve_type(definition->type)
            };

            *alias.resolved_info = resolution::Alias_definition::Resolved_info {
                .resolved    = resolved_alias,
                .type_handle = resolved_alias->type
            };
        }

        auto operator()(resolution::Implementation_definition implementation) -> void {
            assert(!implementation.has_been_resolved());
            // It should be impossible for the implementation to be resolved at this point, because
            // there is no way to refer to an implementation block or its contents before it has been resolved

            auto* const definition = implementation.syntactic_definition;

            bu::wrapper auto type = context.resolve_type(definition->type);


            if (auto space = context.get_associated_namespace_if(type)) {
                bu::wrapper auto associated_namespace     = *space;
                bu::wrapper auto implementation_namespace = implementation.home_namespace;

                implementation_namespace->parent = associated_namespace;

                operator()(implementation_namespace);

                {
                    // Quick & dirty solution, fix later

                    auto& old_lower = associated_namespace    ->lower_table.container();
                    auto& new_lower = implementation_namespace->lower_table.container();

                    old_lower.insert(old_lower.end(), new_lower.begin(), new_lower.end());

                    auto& old_upper = associated_namespace    ->upper_table.container();
                    auto& new_upper = implementation_namespace->upper_table.container();

                    old_upper.insert(old_upper.end(), new_upper.begin(), new_upper.end());

                    auto& old_children = associated_namespace    ->children.container();
                    auto& new_children = implementation_namespace->children.container();

                    old_children.insert(old_children.end(), new_children.begin(), new_children.end());
                }
            }
            else {
                throw context.error(
                    definition->type->source_view,
                    std::format(
                        "{} is not a user-defined type, so it can "
                        "not have associated implementation blocks",
                        type
                    ),
                    "Consider defining a class with the necessary "
                    "members, and instantiating it for the type"
                );
            }
        }

        auto operator()(bu::Wrapper<resolution::Namespace> const child) -> void {
            bu::wrapper auto current_namespace = std::exchange(context.current_namespace, child);

            for (auto& definition : child->definitions_in_order) {
                context.resolve_definition(definition);
            }

            context.current_namespace = current_namespace;
        }

        template <class T>
        auto operator()(resolution::Definition<ast::definition::Template_definition<T>>) -> void {
            // typecheck the templates here
        }

        auto operator()(auto&) -> void {
            bu::unimplemented();
        }
    };

}


auto resolution::Resolution_context::resolve_definition(Definition_variant const variant) -> void {
    std::visit(Definition_resolution_visitor { *this }, variant);
}


auto resolution::resolve(ast::Module&& module) -> ir::Program {
    handle_imports(module);

    auto global_namespace = make_namespace(
        std::nullopt,
        std::nullopt,
        &module.source,
        module.definitions
    );

    {
        resolution::Resolution_context global_context {
            .scope                       { .parent = nullptr },
            .current_namespace           = global_namespace,
            .global_namespace            = global_namespace,
            .source                      = &module.source,
            .is_unevaluated              = false
        };
        Definition_resolution_visitor { global_context } (global_namespace);
    }

    auto* const entry = global_namespace->lower_table.find(lexer::Identifier { "main"sv });
    if (entry) {
        auto& function = std::get<resolution::Function_definition>(*entry);
        bu::print("entry: {}\n", function.resolved_info->value().resolved->body);
    }
    else {
        bu::print("no entry point defined\n");
    }

    // vvv Release all memory used by the AST

    bu::Wrapper<ast::Expression>::release_wrapped_memory();
    bu::Wrapper<ast::Type      >::release_wrapped_memory();
    bu::Wrapper<ast::Pattern   >::release_wrapped_memory();
    bu::Wrapper<ast::Definition>::release_wrapped_memory();

    // ^^^ Fix, use RAII or find wrapped types programmatically ^^^

    return {};
}