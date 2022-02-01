#pragma once

#include "bu/utilities.hpp"


namespace vm {

    enum class Opcode : bu::U8 {
        ipush , fpush , cpush , push_true, push_false,
        idup  , fdup  , cdup  , bdup  ,
        iprint, fprint, cprint, bprint,

        iadd, fadd, cadd,
        isub, fsub, csub,
        imul, fmul, cmul,
        idiv, fdiv, cdiv,
        //iexp, fexp, cexp,

        jump, jump_true, jump_false,

        halt,

        _opcode_count
    };

    auto argument_bytes(Opcode) noexcept -> bu::Usize;

}