#include "bu/utilities.hpp"
#include "tests.hpp"


auto run_lexer_tests () -> void;
auto run_parser_tests() -> void;
auto run_vm_tests    () -> void;


auto tests::run_all_tests() -> void {
    run_lexer_tests();
    run_parser_tests();
    run_vm_tests();

    bu::print("All tests passed!\n");
}