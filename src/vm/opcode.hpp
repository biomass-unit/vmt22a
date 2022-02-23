#pragma once

#include "bu/utilities.hpp"


namespace vm {

    enum class Opcode : bu::U8 {
        ipush , fpush , cpush , push_true, push_false,
        idup  , fdup  , cdup  , bdup  ,
        iprint, fprint, cprint, bprint,

        iadd, fadd,
        isub, fsub,
        imul, fmul,
        idiv, fdiv,

        iinc_top,

        ieq , feq , ceq , beq ,
        ineq, fneq, cneq, bneq,
        ilt , flt ,
        ilte, flte,
        igt , fgt ,
        igte, fgte,

        ieq_i , feq_i , ceq_i , beq_i ,
        ineq_i, fneq_i, cneq_i, bneq_i,
        ilt_i , flt_i ,
        ilte_i, flte_i,
        igt_i , fgt_i ,
        igte_i, fgte_i,

        land, lnand, lor, lnor, lnot,

        cast_itof, cast_ftoi,
        cast_itoc, cast_ctoi,
        cast_itob, cast_btoi,
        cast_ftob,
        cast_ctob,

        bitcopy_from_stack,
        bitcopy_to_stack,
        push_address,
        push_return_value_address,

        jump,       local_jump,
        jump_true,  local_jump_true,
        jump_false, local_jump_false,

        call, call_0, ret,

        halt,

        _opcode_count
    };

    auto argument_bytes(Opcode) noexcept -> bu::Usize;

}