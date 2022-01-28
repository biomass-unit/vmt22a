#pragma once

#include "bu/utilities.hpp"
#include "bu/bytestack.hpp"
#include "bytecode.hpp"


namespace vm {

    struct [[nodiscard]] Virtual_machine {
        bu::Bytestack stack;
        Bytecode      bytecode;
        std::byte*    instruction_pointer = nullptr;
        bool          keep_running        = true;

        explicit Virtual_machine(bu::Usize stack_size) noexcept;

        auto run() -> int;

        template <bu::trivial T>
        auto extract_argument() noexcept -> T;
    };

}