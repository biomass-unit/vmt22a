#pragma once

#include "bu/utilities.hpp"
#include "bu/wrapper.hpp"
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

#undef DEFINE_NODE_CTOR


namespace ast {

    inline Expression const unit_value =       Tuple {};
    inline Type       const unit_type  = type::Tuple {};

    struct [[nodiscard]] Module {
        std::vector<definition::Function>          function_definitions;
        std::vector<definition::Function_template> function_template_definitions;
        std::vector<definition::Data>              data_definitions;
        std::vector<definition::Data_template>     data_template_definitions;
        std::vector<definition::Struct>            struct_definitions;
        std::vector<definition::Struct_template>   struct_template_definitions;
        DEFAULTED_EQUALITY(Module);
    };

}