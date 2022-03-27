#include "bu/utilities.hpp"
#include "resolution.hpp"
#include "resolution_internals.hpp"

#include "lexer/lexer.hpp"
#include "parser/parser.hpp"


auto compiler::Resolution_scope::make_child() noexcept -> Resolution_scope {
    return { .parent = this, .current_frame_offset = current_frame_offset };
}

auto compiler::Resolution_scope::find(lexer::Identifier const name) noexcept -> Binding* {
    if (auto* const pointer = bindings.find(name)) {
        return pointer;
    }
    else {
        return parent ? parent->find(name) : nullptr;
    }
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

    auto make_namespace(std::span<ast::Definition> const definitions) -> compiler::Namespace {
        compiler::Namespace space;

        for (auto& definition : definitions) {
            using namespace ast::definition;

            std::visit(bu::Overload {
                [&](Function& function) -> void {
                    space.lower_table.add(bu::copy(function.name), &function);
                },
                [&](bu::one_of<Struct, Data, Alias, Typeclass> auto& definition) -> void {
                    space.upper_table.add(bu::copy(definition.name), &definition);
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
                    space.children.add(bu::copy(nested_space.name), make_namespace(nested_space.definitions));
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
        .scope             = { .parent = nullptr },
        .current_namespace = &global_namespace,
        .global_namespace  = &global_namespace,
        .is_unevaluated    = false
    };


    // vvv Release all memory used by the AST

    bu::Wrapper<ast::Expression>::release_wrapped_memory();
    bu::Wrapper<ast::Type      >::release_wrapped_memory();
    bu::Wrapper<ast::Pattern   >::release_wrapped_memory();
    bu::Wrapper<ast::Definition>::release_wrapped_memory();

    // ^^^ Fix, use RAII or find wrapped types programmatically ^^^

    return {};
}