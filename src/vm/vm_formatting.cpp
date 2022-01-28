#include "bu/utilities.hpp"
#include "vm_formatting.hpp"


namespace {

    constexpr auto strings = std::to_array<std::string_view>({
        "ipush", "fpush", "cpush", "push_true", "push_false",

        "iadd", "fadd", "cadd",
        "isub", "fsub", "csub",
        "imul", "fmul", "cmul",
        "idiv", "fdiv", "cdiv",
        "iexp", "fexp", "cexp",
    });

    static_assert(strings.size() == static_cast<bu::Usize>(vm::Opcode::_opcode_count));

}


DEFINE_FORMATTER_FOR(vm::Opcode) {
    return std::format_to(context.out(), strings[static_cast<bu::Usize>(value)]);
}