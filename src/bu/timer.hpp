#pragma once

#include <chrono>

#include "bu/utilities.hpp"
#include "bu/color.hpp"


namespace bu {

    template <class Duration = std::chrono::milliseconds>
    struct Timer {
        using Clock = std::chrono::steady_clock;

        Clock::time_point start;
        bool log_scope_exit;

        explicit Timer(bool const log_scope_exit = true) noexcept
            : start { now() }
            , log_scope_exit { log_scope_exit } {}

        ~Timer() {
            if (log_scope_exit) {
                print(
                    "[{}bu::Timer::~Timer{}]: Total elapsed time: {}\n",
                    Color::purple,
                    Color::white,
                    elapsed()
                );
            }
        }

        auto elapsed() const noexcept -> Duration {
            return std::chrono::duration_cast<Duration>(now() - start);
        }

        static auto now() noexcept -> Clock::time_point {
            return Clock::now();
        }
    };

}