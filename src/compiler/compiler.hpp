#pragma once

#include "resolution/ir.hpp"
#include "vm/virtual_machine.hpp"


namespace compiler {

    auto codegen(ir::Program&&) -> vm::Virtual_machine;

}