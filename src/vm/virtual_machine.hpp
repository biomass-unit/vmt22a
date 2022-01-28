#pragma once

#include "bu/utilities.hpp"
#include "bu/bytestack.hpp"
#include "bytecode.hpp"


namespace vm {

    using Jump_offset_type = bu::Usize;

    struct [[nodiscard]] Virtual_machine {
        Bytecode      bytecode;
        bu::Bytestack stack;
        std::byte*    instruction_pointer = nullptr;
        std::byte*    instruction_anchor  = nullptr;
        bool          keep_running        = true;

        auto run() -> int;

        auto jump_to(Jump_offset_type) noexcept -> void;

        template <bu::trivial T>
        auto extract_argument() noexcept -> T;
    };

}