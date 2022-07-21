#pragma once

#include "bu/utilities.hpp"
#include "bu/wrapper.hpp"


namespace mir {

    struct [[nodiscard]] Expression;
    struct [[nodiscard]] Pattern;
    struct [[nodiscard]] Type;

    using Node_context = bu::Wrapper_context_for<Expression, Type, Pattern>;

}


DECLARE_FORMATTER_FOR(mir::Expression);
DECLARE_FORMATTER_FOR(mir::Pattern);
DECLARE_FORMATTER_FOR(mir::Type);