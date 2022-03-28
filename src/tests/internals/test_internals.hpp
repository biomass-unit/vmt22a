#pragma once

#include "bu/utilities.hpp"
#include "bu/color.hpp"


namespace tests {

    struct Test {
        std::string_view name;

        struct Invoke {
            std::function<void()> callable;
            std::source_location  caller;

            Invoke(auto&& callable, std::source_location caller = std::source_location::current())
                : callable { std::forward<decltype(callable)>(callable) }
                , caller { caller } {}
        };

        auto operator=(Invoke&& test) -> void try {
            test.callable();
        }
        catch (std::exception const& exception) {
            bu::print(
                "Test case in [{}.{}] failed: {}\n",
                test.caller.function_name(),
                name,
                exception.what()
            );
        }
    };


    auto assert_eq(auto const& a, auto const& b) -> void {
        if (a != b) {
            throw std::runtime_error {
                std::format(
                    "{}{}{} != {}{}{}",
                    bu::Color::red,
                    a,
                    bu::Color::white,
                    bu::Color::red,
                    b,
                    bu::Color::white
                )
            };
        }
    }


    inline auto operator"" _test(char const* const s, bu::Usize const n) noexcept -> Test {
        return { .name { s, n } };
    }

}