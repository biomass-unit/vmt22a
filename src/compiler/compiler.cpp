#include "bu/utilities.hpp"
#include "compiler.hpp"
#include "codegen_internals.hpp"


namespace {

    struct Codegen_visitor {
        compiler::Codegen_context& context;

        auto operator()(auto&&) -> void {
            bu::unimplemented();
        }
    };

}


auto compiler::codegen(ir::Program&&) -> vm::Virtual_machine {
    bu::unimplemented();
}