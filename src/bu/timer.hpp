#pragma once

#include <chrono>

#include "bu/utilities.hpp"
#include "bu/color.hpp"


namespace bu {

    template <class Duration_ = std::chrono::milliseconds>
    struct Timer {
        using Clock    = std::chrono::steady_clock;
        using Duration = Duration_;

        std::function<void(Duration)> scope_exit_logger = [](Duration const time) {
            print(
                "[{}bu::Timer::~Timer{}]: Total elapsed time: {}\n",
                Color::purple,
                Color::white,
                time
            );
        };

        Clock::time_point start = Clock::now();
        bool              log_scope_exit = true;

        ~Timer() {
            if (log_scope_exit) {
                if (scope_exit_logger) {
                    scope_exit_logger(elapsed());
                }
                else {
                    bu::abort("bu::Timer: log_scope_exit was true, but the scope_exit_logger was uninitialized");
                }
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