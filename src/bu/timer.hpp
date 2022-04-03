#pragma once

#include <chrono>

#include "bu/utilities.hpp"
#include "bu/color.hpp"


namespace bu {

    template <class Duration = std::chrono::milliseconds>
    struct Timer {
        using Clock = std::chrono::steady_clock;

        Clock::time_point start          = Clock::now();
        bool              log_scope_exit = false;

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

        auto restart(Clock::time_point const new_start = Clock::now()) noexcept -> void {
            start = new_start;
        }

        auto elapsed() const noexcept -> Duration {
            return std::chrono::duration_cast<Duration>(Clock::now() - start);
        }
    };

}