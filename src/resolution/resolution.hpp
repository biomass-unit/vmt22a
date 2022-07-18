#pragma once

#include "bu/utilities.hpp"
#include "hir/hir.hpp"
#include "vm/bytecode.hpp"


namespace resolution {

    struct Module {
        struct Interface {
            // TODO
        };
        struct Code {
            vm::Bytecode           bytecode;
            std::vector<bu::Usize> module_address_offsets; // Keeps track of module offsets used in the code
        };

        Interface inferface;
        Code      code;
    };

    auto resolve(hir::Module&&) -> Module;

}