#include "bu/utilities.hpp"
#include "codegen.hpp"
#include "codegen_internals.hpp"


namespace {

    struct Codegen_visitor {
        compiler::Codegen_context& context;

        auto operator()(auto&&) -> void {
            bu::unimplemented();
        }
    };

}


auto codegen::generate(ir::Program&&) -> vm::Virtual_machine {
    bu::unimplemented();
}