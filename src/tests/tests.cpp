#include "bu/utilities.hpp"
#include "bu/color.hpp"
#include "tests.hpp"


namespace {

    constinit bu::Usize success_count = 0;
    constinit bu::Usize test_count    = 0;

    auto red_note() -> std::string_view {
        static auto const note = std::format("{}NOTE:{}", bu::Color::red, bu::Color::white);
        return note;
    }

    auto test_vector() noexcept -> std::vector<void(*)()>& { // Avoids SIOF
        static std::vector<void(*)()> vector;
        return vector;
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


tests::dtl::Test_adder::Test_adder(void(* const test)()) {
    test_vector().push_back(test);
}


auto tests::run_all_tests() -> void {
    using namespace std::chrono;

    auto const start_time = steady_clock::now();

    for (auto const test : test_vector()) {
        test();
    }

    if (success_count == test_count) {
        bu::print(
            "{}All {} tests passed! ({}){}\n",
            bu::Color::green,
            test_count,
            duration_cast<milliseconds>(steady_clock::now() - start_time),
            bu::Color::white
        );
    }
}