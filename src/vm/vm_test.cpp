#include "bu/utilities.hpp"
#include "vm_test.hpp"
#include "opcode.hpp"
#include "virtual_machine.hpp"


namespace {

    auto test(int const expected_value, bu::trivial auto const... program) -> void {
        vm::Virtual_machine machine { .stack = bu::Bytestack { 256 } };
        machine.bytecode.bytes.reserve((sizeof program + ...));
        machine.bytecode.write(program...);

        auto const result = machine.run();

        if (result != expected_value) {
            bu::abort(
                std::format(
                    "VM test case failed, with\n\texpected"
                    " value: {}\n\tactual result: {}",
                    expected_value,
                    result
                )
            );
        }
    }

}


auto vm::run_tests() -> void {
    using enum Opcode;
    using namespace bu::literals;

    test
    (
        10,

        ipush, 2_iz,
        ipush, 5_iz,
        imul,
        halt
    );

    test
    (
        100,

        ipush, 2_iz,
        ipush, 4_iz,
        imul,

        ipush, 5_iz,
        ipush, 5_iz,
        iadd,

        imul,

        ipush, 10_iz,
        ipush, 6_iz,
        isub,
        ipush, 5_iz,
        imul,

        iadd,
        halt
    );

    bu::print("VM tests passed!\n");
}