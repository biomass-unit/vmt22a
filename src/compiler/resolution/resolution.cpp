#include "bu/utilities.hpp"
#include "resolution.hpp"

#include "lexer/lexer.hpp"
#include "parser/parser.hpp"


namespace {

    auto process_imports(std::span<ast::Import> const imports) -> std::vector<ast::Module> {
        auto project_root = std::filesystem::current_path() / "sample-project"; // for debugging purposes, programmatically find path later

        std::vector<ast::Module> modules;

        for (auto& import : imports) {
            auto directory = project_root;

            for (auto& component : import.path | std::views::take(import.path.size() - 1)) {
                directory /= component.view();
            }

            auto path = directory / (std::string { import.path.back().view() } + ".vmt"); // fix later
            modules.push_back(parser::parse(lexer::lex(bu::Source { path.string() })));
        }

        return modules;
    }

}


auto compiler::resolve(ast::Module&& module) -> ir::Program {
    auto imported_modules = process_imports(module.imports);
    (void)imported_modules;

    bu::unimplemented();

    // Perform imports, scope resolution, type
    // checking, turn names into actual references, etc.
}