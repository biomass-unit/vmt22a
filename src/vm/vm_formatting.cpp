#include "bu/utilities.hpp"
#include "virtual_machine.hpp"
#include "vm_formatting.hpp"


namespace {

    constexpr auto opcode_strings = std::to_array<std::string_view>({
        "ipush" , "fpush" , "cpush" , "spush" , "push_true", "push_false",
        "idup"  , "fdup"  , "cdup"  , "sdup"  , "bdup"  ,
        "iprint", "fprint", "cprint", "sprint", "bprint",

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

        "local_jump_ieq_i" , "local_jump_feq_i" , "local_jump_ceq_i" , "local_jump_beq_i" ,
        "local_jump_ineq_i", "local_jump_fneq_i", "local_jump_cneq_i", "local_jump_bneq_i",
        "local_jump_ilt_i" , "local_jump_flt_i" ,
        "local_jump_ilte_i", "local_jump_flte_i",
        "local_jump_igt_i" , "local_jump_fgt_i" ,
        "local_jump_igte_i", "local_jump_fgte_i",

        "call", "call_0", "ret",

        "halt"
    });

    static_assert(opcode_strings.size() == static_cast<bu::Usize>(vm::Opcode::_opcode_count));


    template <class T>
    auto extract(std::byte const*& start, std::byte const* stop) noexcept -> T {
        assert(start + sizeof(T) <= stop);
        (void)stop;
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

        auto const unary = [=, &start]<class T>(bu::Type<T>) {
            return std::format_to(out, "{} {}", opcode, extract<T>(start, stop));
        };

        auto const binary = [=, &start]<class T, class U>(bu::Type<T>, bu::Type<U>) {
            auto const first = extract<T>(start, stop);
            auto const second = extract<U>(start, stop);
            return std::format_to(out, "{} {} {}", opcode, first, second);
        };

        switch (opcode) {
        case vm::Opcode::ipush:
        case vm::Opcode::ieq_i:
        case vm::Opcode::ineq_i:
        case vm::Opcode::ilt_i:
        case vm::Opcode::ilte_i:
        case vm::Opcode::igt_i:
        case vm::Opcode::igte_i:
            return unary(bu::type<bu::Isize>);

        case vm::Opcode::fpush:
        case vm::Opcode::feq_i:
        case vm::Opcode::fneq_i:
        case vm::Opcode::flt_i:
        case vm::Opcode::flte_i:
        case vm::Opcode::fgt_i:
        case vm::Opcode::fgte_i:
            return unary(bu::type<bu::Float>);

        case vm::Opcode::cpush:
        case vm::Opcode::ceq_i:
        case vm::Opcode::cneq_i:
            return unary(bu::type<bu::Char>);

        case vm::Opcode::beq_i:
        case vm::Opcode::bneq_i:
            return unary(bu::type<bool>);

        case vm::Opcode::bitcopy_from_stack:
        case vm::Opcode::bitcopy_to_stack:
            return unary(bu::type<vm::Local_size_type>);

        case vm::Opcode::push_address:
            return unary(bu::type<vm::Local_offset_type>);

        case vm::Opcode::jump:
        case vm::Opcode::jump_true:
        case vm::Opcode::jump_false:
        case vm::Opcode::call_0:
            return unary(bu::type<vm::Jump_offset_type>);

        case vm::Opcode::local_jump:
        case vm::Opcode::local_jump_true:
        case vm::Opcode::local_jump_false:
            return unary(bu::type<vm::Local_offset_type>);

        case vm::Opcode::local_jump_ieq_i:
        case vm::Opcode::local_jump_ineq_i:
        case vm::Opcode::local_jump_ilt_i:
        case vm::Opcode::local_jump_ilte_i:
        case vm::Opcode::local_jump_igt_i:
        case vm::Opcode::local_jump_igte_i:
            return binary(bu::type<vm::Local_offset_type>, bu::type<bu::Isize>);

        case vm::Opcode::local_jump_feq_i:
        case vm::Opcode::local_jump_fneq_i:
        case vm::Opcode::local_jump_flt_i:
        case vm::Opcode::local_jump_flte_i:
        case vm::Opcode::local_jump_fgt_i:
        case vm::Opcode::local_jump_fgte_i:
            return binary(bu::type<vm::Local_offset_type>, bu::type<bu::Float>);

        case vm::Opcode::local_jump_ceq_i:
        case vm::Opcode::local_jump_cneq_i:
            return binary(bu::type<vm::Local_offset_type>, bu::type<bu::Char>);

        case vm::Opcode::local_jump_beq_i:
        case vm::Opcode::local_jump_bneq_i:
            return binary(bu::type<vm::Local_offset_type>, bu::type<bool>);

        case vm::Opcode::call:
            return binary(bu::type<vm::Local_size_type>, bu::type<vm::Jump_offset_type>);

        default:
            assert(vm::argument_bytes(opcode) == 0);
            return std::format_to(out, "{}", opcode);
        }
    }

}


DEFINE_FORMATTER_FOR(vm::Opcode) {
    return std::format_to(context.out(), "{}", opcode_strings[static_cast<bu::Usize>(value)]);
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