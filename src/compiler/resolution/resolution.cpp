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

    auto make_namespace(std::span<ast::Definition> const definitions) -> compiler::Namespace {
        compiler::Namespace space;

        for (auto& definition : definitions) {
            std::visit(bu::Overload {
                [&](ast::definition::Struct& structure) -> void {
                    space.struct_definitions.add(bu::copy(structure.name), &structure);
                },
                [&](ast::definition::Namespace& nested_space) -> void {
                    space.children.add(bu::copy(nested_space.name), make_namespace(nested_space.definitions));
                },
                [](auto&) -> void {
                    bu::unimplemented();
                }
            }, definition.value);
        }

        return space;
    }

}


auto compiler::resolve(ast::Module&& module) -> ir::Program {
    handle_imports(module);

    auto global_namespace = make_namespace(module.definitions);
    Resolution_context context {
        .current_namespace = &global_namespace,
        .global_namespace = &global_namespace
    };


    // vvv Release all memory used by the AST

    bu::Wrapper<ast::Expression>::release_wrapped_memory();
    bu::Wrapper<ast::Type      >::release_wrapped_memory();
    bu::Wrapper<ast::Pattern   >::release_wrapped_memory();

    // ^^^ Fix, use RAII or find wrapped types programmatically ^^^

    return {};
}