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

}


auto compiler::resolve(ast::Module&& module) -> ir::Program {
    handle_imports(module);

    // vvv Release all memory used by the AST

    bu::Wrapper<ast::Expression>::release_wrapped_memory();
    bu::Wrapper<ast::Type      >::release_wrapped_memory();
    bu::Wrapper<ast::Pattern   >::release_wrapped_memory();

    // ^^^ Fix, use RAII or find wrapped types programmatically ^^^

    return {};

    // Perform imports, scope resolution, type
    // checking, turn names into actual references, etc.
}