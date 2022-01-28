#pragma once


namespace vm {

    enum class Opcode : char {
        ipush , fpush , cpush , push_true, push_false,
        idup  , fdup  , cdup  , bdup  ,
        iprint, fprint, cprint, bprint,

        iadd, fadd, cadd,
        isub, fsub, csub,
        imul, fmul, cmul,
        idiv, fdiv, cdiv,
        //iexp, fexp, cexp,

        halt,

        _opcode_count
    };

}