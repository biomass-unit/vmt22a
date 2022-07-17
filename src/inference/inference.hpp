#pragma once

#include "bu/utilities.hpp"
#include "bu/wrapper.hpp"
#include "bu/bounded_integer.hpp"
#include "type.hpp"


namespace inference {

    class Context {
        bu::Wrapper_context<Type> type_context;
        bu::Bounded_usize         current_type_variable_tag;
    public:
        auto fresh_type_variable(type::Variable::Kind = type::Variable::Kind::general) -> type::Variable;
    };

}