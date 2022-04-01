#include "bu/utilities.hpp"
#include "resolution.hpp"
#include "resolution_internals.hpp"

#include "lexer/lexer.hpp"
#include "parser/parser.hpp"


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
        else if (bool* const parameter = mutability_parameters->find(*mutability.parameter_name)) {
            return *parameter;
        }
        else {
            bu::abort("not a mutability parameter");
        }
    default:
        bu::unreachable();
    }
}

auto resolution::Resolution_context::apply_qualifiers(ast::Qualified_name& name) -> bu::Wrapper<Namespace> {
    bu::Wrapper root = std::visit(bu::Overload {
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

        if (bu::Wrapper<Namespace>* const child = root->children.find(qualifier.name)) {
            root = *child;
        }
        else {
            bu::unimplemented();
        }
    }

    return root;
}


auto ir::Type::is_unit() const noexcept -> bool {
    // Checking size == 0 isn't enough, because compound
    // types such as ((), ()) would also count as the unit type

    if (auto* const tuple = std::get_if<ir::type::Tuple>(&value)) {
        return tuple->types.empty();
    }
    else {
        return false;
    }
}


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

    auto make_namespace(std::span<ast::Definition> const definitions) -> bu::Wrapper<resolution::Namespace> {
        bu::Wrapper<resolution::Namespace> space;

        for (auto& definition : definitions) {
            using namespace ast::definition;

            std::visit(bu::Overload {
                [&](Function& function) -> void {
                    resolution::Function_definition handle { &function };
                    space->lower_table.add(bu::copy(function.name), bu::copy(handle));
                    space->definitions_in_order.push_back(handle);
                },
                [&]<bu::one_of<Struct, Data, Alias, Typeclass> T>(T& definition) {
                    resolution::Definition<T> handle { &definition };
                    space->upper_table.add(bu::copy(definition.name), bu::copy(handle));
                    space->definitions_in_order.push_back(handle);
                },

                [](Implementation&) -> void {
                    bu::unimplemented();
                },
                [](Instantiation&) -> void {
                    bu::unimplemented();
                },
                []<class Definition>(Template_definition<Definition>&) -> void {
                    bu::unimplemented();
                },

                [&](Namespace& nested_space) -> void {
                    bu::Wrapper child = make_namespace(nested_space.definitions);
                    space->definitions_in_order.push_back(child);
                    space->children.add(bu::copy(nested_space.name), bu::copy(child));
                }
            }, definition.value);
        }

        return space;
    }


    auto resolve_definitions(bu::Wrapper<resolution::Namespace> const current_namespace,
                             bu::Wrapper<resolution::Namespace> const global_namespace,
                             bu::Source*                        const source) -> void;


    struct Definition_resolution_visitor {
        bu::Wrapper<resolution::Namespace> current_namespace;
        bu::Wrapper<resolution::Namespace> global_namespace;
        bu::Source*                        source;

        auto operator()(resolution::Function_definition function) -> void {
            if (!function.resolved->has_value()) {
                resolution::Resolution_context context {
                    .scope                 { .parent = nullptr },
                    .current_namespace     = current_namespace,
                    .global_namespace      = global_namespace,
                    .source                = source,
                    .mutability_parameters = nullptr,
                    .is_unevaluated        = false,
                };

                auto* const definition = function.syntactic_definition;

                std::vector<ir::definition::Function::Parameter> parameters;
                parameters.reserve(definition->parameters.size());

                for (auto& [pattern, type, default_value] : definition->parameters) {
                    bu::Wrapper ir_type = resolution::resolve_type(type, context);
                    context.bind(pattern, ir_type);

                    parameters.emplace_back(
                        ir_type,
                        default_value.transform([&](ast::Expression& expression) -> bu::Wrapper<ir::Expression> {
                            return resolution::resolve_expression(expression, context);
                        })
                    );
                }

                auto body = resolution::resolve_expression(definition->body, context);

                if (definition->return_type) {
                    if (body.type != resolution::resolve_type(*definition->return_type, context)) {
                        bu::abort("function return type mismatch");
                    }
                }

                std::vector<bu::Wrapper<ir::Type>> parameter_types;
                parameter_types.reserve(parameters.size());

                std::ranges::copy(
                    parameters | std::views::transform(&ir::definition::Function::Parameter::type),
                    std::back_inserter(parameter_types)
                );

                bu::Wrapper return_type = body.type;

                *function.resolved = ir::definition::Function{
                    .name        = std::string { function.syntactic_definition->name.view() },
                    .parameters  = std::move(parameters),
                    .return_type = std::move(return_type),
                    .body        = std::move(body),

                    .function_type = ir::Type {
                        .value = ir::type::Function {
                            .parameter_types = std::move(parameter_types),
                            .return_type     = return_type
                        },
                        .size = ir::Size_type { bu::unchecked_tag, 2 * sizeof(std::byte*) }
                    }
                };
            }
        }

        auto operator()(bu::Wrapper<resolution::Namespace> const child) -> void {
            resolve_definitions(child, global_namespace, source);
        }

        auto operator()(auto&) -> void {
            bu::unimplemented();
        }
    };


    auto resolve_definitions(bu::Wrapper<resolution::Namespace> const current_namespace,
                             bu::Wrapper<resolution::Namespace> const global_namespace,
                             bu::Source*                        const source) -> void
    {
        for (auto& definition : current_namespace->definitions_in_order) {
            std::visit(
                Definition_resolution_visitor {
                    .current_namespace = current_namespace,
                    .global_namespace  = global_namespace,
                    .source            = source
                },
                definition
            );
        }
    }

}


auto resolution::resolve(ast::Module&& module) -> ir::Program {
    handle_imports(module);

    auto global_namespace = make_namespace(module.definitions);

    resolve_definitions(global_namespace, global_namespace, &module.source);

    auto* const entry = global_namespace->lower_table.find(lexer::Identifier { "main"sv });
    if (entry) {
        auto& function = std::get<resolution::Function_definition>(*entry);
        bu::print("entry: {}\n", function.resolved->value()->body);
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