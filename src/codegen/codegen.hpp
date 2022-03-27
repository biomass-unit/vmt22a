#pragma once

#include "ir/ir.hpp"
#include "vm/virtual_machine.hpp"


namespace codegen {

    auto generate(ir::Program&&) -> vm::Virtual_machine;

}