#include "bu/utilities.hpp"
#include "virtual_machine.hpp"
#include "vm_formatting.hpp"


namespace {

    constexpr auto opcode_strings = std::to_array<std::string_view>({
        "ipush" , "fpush" , "cpush" , "push_true", "push_false",
        "idup"  , "fdup"  , "cpush" , "bpush" ,
        "iprint", "fprint", "cprint", "bprint",

        "iadd", "fadd", "cadd",
        "isub", "fsub", "csub",
        "imul", "fmul", "cmul",
        "idiv", "fdiv", "cdiv",

        "iinc_top",

        "ieq" , "feq" , "ceq" , "beq" ,
        "ineq", "fneq", "cneq", "bneq",
        "ilt" , "flt" , "clt" ,
        "ilte", "flte", "clte",
        "igt" , "fgt" , "cgt" ,
        "igte", "fgte", "cgte",

        "land", "lnand", "lor", "lnor", "lnot",

        "bitcopy_from_stack",
        "bitcopy_to_stack",
        "push_address",
        "push_return_value",

        "jump", "jump_true", "jump_false",

        "call", "ret",

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
            return unary(bu::typetag<bu::Isize>);
        case fpush:
            return unary(bu::typetag<bu::Float>);
        case cpush:
            return unary(bu::typetag<char>);
        case bitcopy_from_stack:
        case bitcopy_to_stack:
            return unary(bu::typetag<vm::Local_size_type>);
        case push_address:
            return unary(bu::typetag<vm::Local_offset_type>);
        case jump:
        case jump_true:
        case jump_false:
            return unary(bu::typetag<vm::Jump_offset_type>);
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