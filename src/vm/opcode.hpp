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

        ieq , feq , ceq , beq ,
        ineq, fneq, cneq, bneq,
        ilt , flt , clt ,
        ilte, flte, clte,
        igt , fgt , cgt ,
        igte, fgte, cgte,

        land, lnand, lor, lnor, lnot,

        jump, jump_true, jump_false,

        call, ret,

        halt,

        _opcode_count
    };

    auto argument_bytes(Opcode) noexcept -> bu::Usize;

}