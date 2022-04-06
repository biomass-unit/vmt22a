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

    auto make_namespace(std::optional<bu::Wrapper<resolution::Namespace>> parent,
                        std::optional<lexer::Identifier>                  name,
                        std::span<ast::Definition> const                  definitions)
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

            std::visit(bu::Overload {
                [&](Function& function) -> void {
                    resolution::Function_definition handle {
                        .syntactic_definition = &function,
                        .home_namespace       = space
                    };
                    space->definitions_in_order.push_back(handle);
                    space->lower_table.add(bu::copy(function.name), bu::copy(handle));
                },
                [&]<bu::one_of<Struct, Data, Alias, Typeclass> T>(T& definition) -> void {
                    resolution::Definition<T> handle {
                        .syntactic_definition = &definition,
                        .home_namespace       = space
                    };
                    space->definitions_in_order.push_back(handle);
                    space->upper_table.add(bu::copy(definition.name), bu::copy(handle));
                },
                [&]<bu::one_of<Struct, Data, Alias, Typeclass> T>(Template_definition<T>& template_definition) -> void {
                    resolution::Definition<Template_definition<T>> handle {
                        .syntactic_definition = &template_definition.definition,
                        .home_namespace       = space,
                        .template_parameters  = template_definition.parameters
                    };
                    space->definitions_in_order.push_back(handle);
                    space->upper_table.add(bu::copy(template_definition.definition.name), bu::copy(handle));
                },

                [](Implementation&) -> void {
                    bu::unimplemented();
                },
                [](Instantiation&) -> void {
                    bu::unimplemented();
                },

                [&](Namespace& nested_space) -> void {
                    bu::wrapper auto child = make_namespace(space, nested_space.name, nested_space.definitions);
                    space->definitions_in_order.push_back(child);
                    space->children.add(bu::copy(nested_space.name), bu::copy(child));
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

        auto operator()(resolution::Function_definition function) -> void {
            if (function.has_been_resolved()) {
                return;
            }

            auto* const definition = function.syntactic_definition;

            std::vector<ir::definition::Function::Parameter> parameters;
            parameters.reserve(definition->parameters.size());

            for (auto& [pattern, type, default_value] : definition->parameters) {
                bu::wrapper auto ir_type = resolution::resolve_type(type, context);
                context.bind(pattern, ir_type);

                parameters.emplace_back(
                    ir_type,
                    default_value.transform([&](ast::Expression& expression) -> bu::Wrapper<ir::Expression> {
                        return resolution::resolve_expression(expression, context);
                    })
                );
            }

            bu::Wrapper body = resolution::resolve_expression(definition->body, context);

            if (definition->return_type) {
                if (body->type != resolution::resolve_type(*definition->return_type, context)) {
                    bu::abort("function return type mismatch");
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
                    .name        = context.current_namespace->format_name_as_member(definition->name),
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
                bu::wrapper auto    type   = resolution::resolve_type(member.type, context);
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
                .members    = std::move(members),
                .name       = context.current_namespace->format_name_as_member(definition->name),
                .size       = size,
                .is_trivial = is_trivial
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

        auto operator()(bu::Wrapper<resolution::Namespace> const child) -> void {
            bu::wrapper auto current_namespace = std::exchange(context.current_namespace, child);

            for (auto& definition : child->definitions_in_order) {
                resolution::resolve_definition(definition, context);
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


auto resolution::resolve_definition(Definition_variant const variant, Resolution_context& context) -> void {
    std::visit(Definition_resolution_visitor { context }, variant);
}


auto resolution::resolve(ast::Module&& module) -> ir::Program {
    handle_imports(module);

    auto global_namespace = make_namespace(std::nullopt, std::nullopt, module.definitions);

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