#pragma once

#include "bu/utilities.hpp"
#include "bu/source.hpp"


namespace mir {

    struct [[nodiscard]] Expression;
    struct [[nodiscard]] Pattern;
    struct [[nodiscard]] Type;

    struct [[nodiscard]] Function_parameter;

    struct [[nodiscard]] Template_type_parameter;
    struct [[nodiscard]] Template_mutability_parameter;
    struct [[nodiscard]] Template_value_parameter;

    struct Template_parameter_set {
        std::vector<Template_type_parameter>       type_parameters;
        std::vector<Template_mutability_parameter> mutability_parameters;
        std::vector<Template_value_parameter>      value_parameters;
    };

}


#include "nodes/type.hpp"
#include "nodes/expression.hpp"
#include "nodes/pattern.hpp"
#include "nodes/definition.hpp"


struct mir::Template_type_parameter {

};

struct mir::Template_mutability_parameter {

};

struct mir::Template_value_parameter {

};


struct mir::Function_parameter {
    Pattern pattern;
    Type    type;
};


namespace mir {

    using Node_context = bu::Wrapper_context_for<Expression, Pattern, Type>;

}