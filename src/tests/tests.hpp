#pragma once

#include "bu/utilities.hpp"
#include "bu/color.hpp"


namespace tests {

    auto run_all_tests() -> void;


    struct Failure : bu::Exception {
        std::source_location thrower;

        explicit Failure(std::string&& message,
                         std::source_location = std::source_location::current());
    };
    

    struct Test {
        std::string_view name;
        bool             should_fail;

        struct Invoke {
            std::function<void()> callable;
            std::source_location  caller;

            Invoke(auto&&               callable,
                   std::source_location caller = std::source_location::current()
            )
                : callable { std::forward<decltype(callable)>(callable) }
                , caller { caller } {}
        };

        auto operator=(Invoke&&) -> void;
    };


    auto assert_eq(auto const& a,
                   auto const& b,
                   std::source_location caller = std::source_location::current())
    {
        if (a != b) {
            throw Failure {
                std::format(
                    "{}{}{} != {}{}{}",
                    bu::Color::red,
                    a,
                    bu::Color::white,
                    bu::Color::red,
                    b,
                    bu::Color::white
                ),
                caller
            };
        }
    }


    inline auto operator"" _test(char const* const s, bu::Usize const n) noexcept -> Test {
        return { .name { s, n }, .should_fail = false };
    }

    inline auto operator"" _failing_test(char const* const s, bu::Usize const n) noexcept -> Test {
        return { .name { s, n }, .should_fail = true };
    }


    namespace dtl {
        struct Test_adder {
            explicit Test_adder(void(*)());
        };
    }

}


#define REGISTER_TEST_IMPL_IMPL(test_function, line) \
namespace test_impl { static tests::dtl::Test_adder test_adder_ ## line { &test_function }; }

#define REGISTER_TEST_IMPL(test_function, line) REGISTER_TEST_IMPL_IMPL(test_function, line)
#define REGISTER_TEST(test_function) REGISTER_TEST_IMPL(test_function, __LINE__)