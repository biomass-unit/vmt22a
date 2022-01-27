#pragma once

#include "bu/utilities.hpp"
#include "bu/wrapper.hpp"
#include "lexer/token.hpp"


namespace ast {

    struct [[nodiscard]] Expression;
    struct [[nodiscard]] Pattern;
    struct [[nodiscard]] Type;
    struct [[nodiscard]] Definition;

    struct [[nodiscard]] Module {
        // todo
    };

}


#define DEFINE_NODE_CTOR(name)                                            \
template <class X> requires (!bu::similar_to<X, name>)                    \
name(X&& x) noexcept(std::is_nothrow_constructible_v<Variant, X&&>) \
    : value { std::forward<X>(x) } {}

#include "nodes/expression.hpp"
#include "nodes/pattern.hpp"
#include "nodes/type.hpp"
#include "nodes/definition.hpp"

#undef DEFINE_NODE_CTOR


namespace ast {

    inline Expression unit_value = Tuple {};
    inline Type unit_type = type::Tuple {};

}