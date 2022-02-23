#include "bu/utilities.hpp"
#include "virtual_machine.hpp"
#include "vm_formatting.hpp"


namespace {

    constexpr auto opcode_strings = std::to_array<std::string_view>({
        "ipush" , "fpush" , "cpush" , "push_true", "push_false",
        "idup"  , "fdup"  , "cdup"  , "bdup"  ,
        "iprint", "fprint", "cprint", "bprint",

        "iadd", "fadd",
        "isub", "fsub",
        "imul", "fmul",
        "idiv", "fdiv",

        "iinc_top",

        "ieq" , "feq" , "ceq" , "beq" ,
        "ineq", "fneq", "cneq", "bneq",
        "ilt" , "flt" ,
        "ilte", "flte",
        "igt" , "fgt" ,
        "igte", "fgte",

        "ieq_i" , "feq_i" , "ceq_i" , "beq_i" ,
        "ineq_i", "fneq_i", "cneq_i", "bneq_i",
        "ilt_i" , "flt_i" ,
        "ilte_i", "flte_i",
        "igt_i" , "fgt_i" ,
        "igte_i", "fgte_i",

        "land", "lnand", "lor", "lnor", "lnot",

        "cast_itof", "cast_ftoi",
        "cast_itoc", "cast_ctoi",

        "cast_itob", "cast_btoi",
        "cast_ftob",
        "cast_ctob",

        "bitcopy_from_stack",
        "bitcopy_to_stack",
        "push_address",
        "push_return_value_address",

        "jump",       "local_jump",
        "jump_true",  "local_jump_true",
        "jump_false", "local_jump_false",

        "call", "call_0", "ret",

        "halt"
    });

    static_assert(opcode_strings.size() == static_cast<bu::Usize>(vm::Opcode::_opcode_count));


    template <class T>
    auto extract(std::byte const*& start, std::byte const* stop) noexcept -> T {
        assert(start + sizeof(T) <= stop);
        std::ignore = stop;
        T x;
        std::memcpy(&x, start, sizeof x);
        start += sizeof x;
        return x;
    }

    auto format_instruction(std::format_context::iterator out,
                            std::byte const*&             start,
                            std::byte const*              stop)
    {
        auto const opcode = extract<vm::Opcode>(start, stop);

        auto unary = [=, &start]<class T>(bu::Typetag<T>) {
            return std::format_to(out, "{} {}", opcode, extract<T>(start, stop));
        };

        using enum vm::Opcode;
        switch (opcode) {
        case ipush:
        case ieq_i:
        case ineq_i:
        case ilt_i:
        case ilte_i:
        case igt_i:
        case igte_i:
            return unary(bu::typetag<bu::Isize>);
        case fpush:
        case feq_i:
        case fneq_i:
        case flt_i:
        case flte_i:
        case fgt_i:
        case fgte_i:
            return unary(bu::typetag<bu::Float>);
        case cpush:
        case ceq_i:
        case cneq_i:
            return unary(bu::typetag<bu::Char>);
        case beq_i:
        case bneq_i:
            return unary(bu::typetag<bool>);
        case bitcopy_from_stack:
        case bitcopy_to_stack:
            return unary(bu::typetag<vm::Local_size_type>);
        case push_address:
            return unary(bu::typetag<vm::Local_offset_type>);
        case jump:
        case jump_true:
        case jump_false:
        case call_0:
            return unary(bu::typetag<vm::Jump_offset_type>);
        case local_jump:
        case local_jump_true:
        case local_jump_false:
            return unary(bu::typetag<vm::Local_offset_type>);
        case call:
        {
            auto const return_value_size = extract<vm::Local_size_type>(start, stop);
            auto const function_offset = extract<vm::Jump_offset_type>(start, stop);
            return std::format_to(out, "{} {} {}", opcode, return_value_size, function_offset);
        }
        default:
            assert(vm::argument_bytes(opcode) == 0);
            return std::format_to(out, "{}", opcode);
        }
    }

}


DEFINE_FORMATTER_FOR(vm::Opcode) {
    return std::format_to(context.out(), opcode_strings[static_cast<bu::Usize>(value)]);
}

DEFINE_FORMATTER_FOR(vm::Bytecode) {
    auto const start = value.bytes.data();
    auto const stop  = start + value.bytes.size();
    auto const out   = context.out();

    auto const digit_count = bu::digit_count(bu::unsigned_distance(start, stop));

    for (auto pointer = start; pointer < stop; ) {
        std::format_to(out, "{:>{}} ", std::distance(start, pointer), digit_count);
        format_instruction(out, pointer, stop);
        std::format_to(out, "\n");
    }

    return out;
}