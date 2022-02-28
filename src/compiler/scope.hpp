#pragma once

#include "bu/utilities.hpp"
#include "bu/wrapper.hpp"
#include "vm/virtual_machine.hpp"


namespace compiler {

    struct Scope {
        vm::Virtual_machine* machine;
        Scope*               parent;

        auto make_child() -> Scope;
    };

}