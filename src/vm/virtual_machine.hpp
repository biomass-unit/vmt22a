#pragma once

#include "bu/utilities.hpp"
#include "bu/bytestack.hpp"


namespace vm {

    struct [[nodiscard]] Virtual_machine {
        bu::Bytestack stack;
        std::byte*    instruction_pointer;
        bool          keep_running;

        auto run() -> int;
    };

}