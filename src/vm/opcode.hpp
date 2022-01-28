#pragma once


namespace vm {

    enum class Opcode : char {
        ipush, fpush, cpush, push_true, push_false,

        iadd, fadd, cadd,
        isub, fsub, csub,
        imul, fmul, cmul,
        idiv, fdiv, cdiv,
        iexp, fexp, cexp,

        _opcode_count
    };

}