#include "bu/utilities.hpp"
#include "compiler.hpp"
#include "codegen_internals.hpp"
#include "ast/ast_formatting.hpp"


namespace {

    struct Codegen_visitor {
        compiler::Codegen_context& context;

        auto operator()(auto const* definition) {
            bu::abort(std::format("Codegen_visitor::operator(): {}", *definition));
        }
    };

}


auto compiler::compile(ast::Module&& module) -> vm::Virtual_machine {
    vm::Virtual_machine machine {
        .stack = bu::Bytestack { 1'000'000 }
    };

    Codegen_context context { machine, std::move(module) };

    context.space->handle_recursively([&](ast::Namespace& space) {
        for (auto const definition : space.definitions_in_order) {
            std::visit(Codegen_visitor { context }, definition);
        }
    });

    return machine;
}