#include "bu/utilities.hpp"
#include "tests.hpp"

#include "vm/opcode.hpp"
#include "vm/virtual_machine.hpp"


namespace {

    auto run_bytecode(bu::trivial auto const... program) -> int {
        vm::Virtual_machine machine { .stack = bu::Bytestack { 256 } };
        machine.bytecode.write(program...);
        return machine.run();
    }

    auto run_vm_tests() -> void {
        using namespace bu::literals;
        using namespace tests;
        using enum vm::Opcode;

        "arithmetic"_test = [] {
            assert_eq(
                10,
                run_bytecode(
                    ipush, 2_iz,
                    ipush, 5_iz,
                    imul,
                    halt
                )
            );

            assert_eq(
                100,
                run_bytecode(
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
                )
            );
        };
    }

}


REGISTER_TEST(run_vm_tests);