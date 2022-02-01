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

        "jump", "jump_true", "jump_false",

        "halt"
    });

    static_assert(opcode_strings.size() == static_cast<bu::Usize>(vm::Opcode::_opcode_count));


    template <class T>
    auto extract(std::byte const*& start, std::byte const* stop) noexcept -> T {
        assert(start + sizeof(T) <= stop);
        T x;
        std::memcpy(&x, start, sizeof x);
        start += sizeof x;
        return x;
    }

    auto format_instruction(std::format_context::iterator out,
                            std::byte const*&             start,
                            std::byte const*              stop)
    {
        auto unary = [=, &start]<class T>(bu::Typetag<T>) {
            return std::format_to(out, "{} {}", extract<T>(start, stop));
        };

        using enum vm::Opcode;
        switch (auto const opcode = extract<vm::Opcode>(start, stop)) {
        case ipush:
            return unary(bu::typetag<bu::Isize>);
        case fpush:
            return unary(bu::typetag<bu::Float>);
        case cpush:
            return unary(bu::typetag<char>);
        case jump:
        case jump_true:
        case jump_false:
            return unary(bu::typetag<vm::Jump_offset_type>);
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