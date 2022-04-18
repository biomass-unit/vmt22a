#include "bu/utilities.hpp"
#include "bu/color.hpp"
#include "tests.hpp"
#include "internals/test_internals.hpp"


namespace {

    constinit bu::Usize success_count = 0;
    constinit bu::Usize test_count    = 0;

    auto red_note() -> std::string_view {
        std::string const note = std::format("{}NOTE:{}", bu::Color::red, bu::Color::white);
        return note;
    }

}


tests::Failure::Failure(std::string&& message, std::source_location const thrower)
    : Exception { std::move(message) }
    , thrower   { thrower } {}


auto tests::Test::operator=(Invoke&& test) -> void {
    auto const test_name = [&] {
        return std::format("[{}.{}]", test.caller.function_name(), this->name);
    };

    ++test_count;

    try {
        test.callable();
    }
    catch (Failure const& failure) {
        if (!should_fail) {
            bu::print(
                "{} Test case failed in {}, on line {}: {}\n",
                red_note(),
                test_name(),
                failure.thrower.line(),
                failure.what()
            );
        }
        else {
            ++success_count;
        }
        return;
    }
    catch (...) {
        if (!should_fail) {
            bu::print(
                "{} Exception thrown during test {}\n",
                red_note(),
                test_name()
            );
            throw;
        }
        else {
            ++success_count;
        }
        return;
    }

    if (should_fail) {
        bu::print(
            "{} Test {} should have failed, but didn't\n",
            red_note(),
            test_name()
        );
    }
    else {
        ++success_count;
    }
}


auto run_lexer_tests  () -> void;
auto run_parser_tests () -> void;
auto run_vm_tests     () -> void;


auto tests::run_all_tests() -> void {
    run_lexer_tests  ();
    run_parser_tests ();
    run_vm_tests     ();

    if (success_count == test_count) {
        bu::print(
            "{}All {} tests passed!{}\n",
            bu::Color::green,
            test_count,
            bu::Color::white
        );
    }
}