#pragma once

#include "bu/utilities.hpp"
#include "bu/wrapper.hpp"
#include "bu/source.hpp"
#include "hir/hir.hpp"
#include "forward.hpp"


namespace mir {

    struct [[nodiscard]] Function_parameter;

    struct [[nodiscard]] Template_type_parameter;
    struct [[nodiscard]] Template_mutability_parameter;
    struct [[nodiscard]] Template_value_parameter;

    struct Template_parameter_set {
        std::vector<Template_type_parameter>       type_parameters;
        std::vector<Template_mutability_parameter> mutability_parameters;
        std::vector<Template_value_parameter>      value_parameters;
    };

    struct Class_reference {};

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