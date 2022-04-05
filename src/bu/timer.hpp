#pragma once

#include <chrono>

#include "bu/utilities.hpp"
#include "bu/color.hpp"


namespace bu {

    template <class Duration_ = std::chrono::milliseconds>
    struct Timer {
        using Clock    = std::chrono::steady_clock;
        using Duration = Duration_;

        Clock::time_point             start             = Clock::now();
        std::function<void(Duration)> scope_exit_logger = [](Duration const time) {
            print(
                "[{}bu::Timer::~Timer{}]: Total elapsed time: {}\n",
                Color::purple,
                Color::white,
                time
            );
        };

        ~Timer() {
            if (scope_exit_logger) {
                scope_exit_logger(elapsed());
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