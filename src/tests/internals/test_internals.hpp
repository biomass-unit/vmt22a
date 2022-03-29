#pragma once

#include "bu/utilities.hpp"
#include "bu/color.hpp"


namespace tests {

    namespace dtl {
        auto red_text(auto const& x) -> std::string {
            return std::format("{}{}{}", bu::Color::red, x, bu::Color::white);
        }
    }


    struct Failure : std::runtime_error {
        using runtime_error::runtime_error;
        using runtime_error::operator=;
    };
    

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
        catch (Failure const& failure) {
            bu::print(
                "{} Test case in [{}.{}] failed: {}\n",
                dtl::red_text("NOTE:"),
                test.caller.function_name(),
                name,
                failure.what()
            );
        }
        catch (...) {
            bu::print(
                "{} Exception thrown during test [{}.{}]\n",
                dtl::red_text("NOTE:"),
                test.caller.function_name(),
                name
            );
            throw;
        }
    };


    auto assert_eq(auto const& a, auto const& b) -> void {
        if (a != b) {
            throw Failure { std::format("{} != {}", dtl::red_text(a), dtl::red_text(b)) };
        }
    }


    inline auto operator"" _test(char const* const s, bu::Usize const n) noexcept -> Test {
        return { .name { s, n } };
    }

}