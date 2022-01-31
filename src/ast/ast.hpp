#pragma once

#include "bu/utilities.hpp"
#include "bu/wrapper.hpp"
#include "bu/source.hpp"
#include "lexer/token.hpp"


namespace ast {

    struct [[nodiscard]] Expression;
    struct [[nodiscard]] Pattern;
    struct [[nodiscard]] Type;

}


#define DEFINE_NODE_CTOR(name)                                      \
template <class X> requires (!bu::similar_to<X, name>)              \
name(X&& x) noexcept(std::is_nothrow_constructible_v<Variant, X&&>) \
    : value { std::forward<X>(x) } {}

#include "nodes/expression.hpp"
#include "nodes/pattern.hpp"
#include "nodes/type.hpp"
#include "nodes/definition.hpp"
#include "nodes/namespace.hpp"

#undef DEFINE_NODE_CTOR


namespace ast {

    inline Expression const unit_value =       Tuple {};
    inline Type       const unit_type  = type::Tuple {};

    struct [[nodiscard]] Module {
        bu::Source source;
        Namespace global_namespace;
    };

    class AST_context {
        bu::Wrapper_context<ast::Expression> expr_context {};
        bu::Wrapper_context<ast::Pattern   > patt_context {};
        bu::Wrapper_context<ast::Type      > type_context {};
    public:
        explicit AST_context(bu::Usize const capacity) noexcept
            : expr_context { capacity }
            , patt_context { capacity }
            , type_context { capacity } {}
    };

}