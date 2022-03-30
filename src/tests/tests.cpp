#include "bu/utilities.hpp"
#include "bu/color.hpp"
#include "tests.hpp"
#include "internals/test_internals.hpp"


auto tests::dtl::tests_failed() noexcept -> bool& {
    static bool state = false;
    return state;
}


auto run_lexer_tests  () -> void;
auto run_parser_tests () -> void;
auto run_vm_tests     () -> void;


auto tests::run_all_tests() -> void {
    run_lexer_tests  ();
    run_parser_tests ();
    run_vm_tests     ();

    if (!dtl::tests_failed()) {
        bu::print("{}All tests passed!{}\n", bu::Color::green, bu::Color::white);
    }
}