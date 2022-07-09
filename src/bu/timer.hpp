#pragma once

#include <chrono>

#include "bu/utilities.hpp"
#include "bu/color.hpp"


namespace bu {

    template <class Clock_type, class Duration_type>
    struct Basic_timer {
        using Clock    = Clock_type;
        using Duration = Duration_type;

        Clock::time_point start = Clock::now();

        auto restart(Clock::time_point const new_start = Clock::now()) noexcept -> void {
            start = new_start;
        }

        auto elapsed() const noexcept -> Duration {
            return std::chrono::duration_cast<Duration>(Clock::now() - start);
        }
    };


    template <class Timer_type>
    struct Basic_logging_timer : Timer_type {
        std::function<void(typename Timer_type::Duration)> scope_exit_logger;

        Basic_logging_timer() noexcept
            : scope_exit_logger
        {
            +[](Timer_type::Duration const time) {
                print(
                    "[{}bu::Logging_timer::~Logging_timer{}]: Total elapsed time: {}\n",
                    Color::purple,
                    Color::white,
                    time
                );
            }
        } {}

        explicit Basic_logging_timer(decltype(scope_exit_logger)&& scope_exit_logger) noexcept
            : scope_exit_logger { std::move(scope_exit_logger) } {}

        ~Basic_logging_timer() {
            if (scope_exit_logger) {
                scope_exit_logger(Timer_type::elapsed());
            }
            else {
                bu::abort("bu::Logging_timer: scope_exit_logger was uninitialized");
            }
        }
    };


    using Timer = Basic_timer<std::chrono::steady_clock, std::chrono::milliseconds>;
    using Logging_timer = Basic_logging_timer<Timer>;

}