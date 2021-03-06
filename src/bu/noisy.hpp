#pragma once

#include "bu/utilities.hpp"


namespace bu {

#define BU_NOISY_LOG(f, ...) \
f noexcept { print("[this: {}] bu::Noisy::" #f "\n", static_cast<void const*>(this)); __VA_ARGS__ }

    struct Noisy {
        BU_NOISY_LOG(Noisy());
        BU_NOISY_LOG(Noisy(Noisy const&));
        BU_NOISY_LOG(Noisy(Noisy&&));
        BU_NOISY_LOG(~Noisy());
        auto BU_NOISY_LOG(operator=(Noisy const&), return *this;);
        auto BU_NOISY_LOG(operator=(Noisy&&), return *this;);
    };

#undef BU_NOISY_LOG

}