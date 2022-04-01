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

auto resolution::Resolution_context::apply_qualifiers(ast::Qualified_name& name) -> Namespace* {
    auto* root = std::visit(bu::Overload {
        [&](std::monostate) {
            return current_namespace;
        },
        [&](ast::Root_qualifier::Global) {
            return global_namespace;
        },
        [](ast::Type&) -> Namespace* {
            bu::unimplemented();
        }
    }, name.root_qualifier->value);

    for (auto& qualifier : name.middle_qualifiers) {
        if (qualifier.is_upper || qualifier.template_arguments) {
            bu::unimplemented();
        }

        if (auto* const child = root->children.find(qualifier.name)) {
            root = child;
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

    auto make_namespace(std::span<ast::Definition> const definitions) -> resolution::Namespace {
        resolution::Namespace space;

        for (auto& definition : definitions) {
            using namespace ast::definition;

            std::visit(bu::Overload {
                [&](Function& function) -> void {
                    resolution::Function_definition handle { &function };
                    space.lower_table.add(bu::copy(function.name), bu::copy(handle));
                    space.definitions_in_order.push_back(handle);
                },
                [&]<bu::one_of<Struct, Data, Alias, Typeclass> T>(T& definition) {
                    resolution::Definition<T> handle { &definition };
                    space.upper_table.add(bu::copy(definition.name), bu::copy(handle));
                    space.definitions_in_order.push_back(handle);
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
                    space.children.add(
                        bu::copy(nested_space.name),
                        make_namespace(nested_space.definitions)
                    );
                }
            }, definition.value);
        }

        return space;
    }


    auto resolve_definitions(ast::Module&           module,
                             resolution::Namespace* global,
                             resolution::Namespace* current) -> void;


    struct Definition_resolution_visitor {
        resolution::Namespace* current_namespace;
        resolution::Namespace* global_namespace;
        ast::Module*           module;

        auto operator()(resolution::Function_definition function) -> void {
            if (!function.resolved->has_value()) {
                resolution::Resolution_context context {
                    .scope                 { .parent = nullptr },
                    .current_namespace     = current_namespace,
                    .global_namespace      = global_namespace,
                    .source                = &module->source,
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

                bu::Wrapper return_type = body.type;

                function.resolved = ir::definition::Function {
                    std::move(parameters),
                    std::move(return_type),
                    std::move(body),
                };
            }
        }

        auto operator()(auto&) -> void {
            bu::unimplemented();
        }
    };


    auto resolve_definitions(ast::Module&                 module,
                             resolution::Namespace* const global_namespace,
                             resolution::Namespace* const current_namespace)
        -> void
    {
        for (auto& definition : current_namespace->definitions_in_order) {
            std::visit(
                Definition_resolution_visitor {
                    .current_namespace = current_namespace,
                    .global_namespace  = global_namespace,
                    .module            = &module
                },
                definition
            );
        }
    }

}


auto resolution::resolve(ast::Module&& module) -> ir::Program {
    handle_imports(module);

    auto global_namespace = make_namespace(module.definitions);

    resolve_definitions(module, &global_namespace, &global_namespace);


    // vvv Release all memory used by the AST

    bu::Wrapper<ast::Expression>::release_wrapped_memory();
    bu::Wrapper<ast::Type      >::release_wrapped_memory();
    bu::Wrapper<ast::Pattern   >::release_wrapped_memory();
    bu::Wrapper<ast::Definition>::release_wrapped_memory();

    // ^^^ Fix, use RAII or find wrapped types programmatically ^^^

    return {};
}