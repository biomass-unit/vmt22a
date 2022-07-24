#pragma once

#include "bu/utilities.hpp"
#include "bu/wrapper.hpp"
#include "bu/source.hpp"
#include "hir/hir.hpp"
#include "forward.hpp"


namespace mir {
    struct Function;
    struct Struct;
    struct Enum;
    struct Alias;
    struct Typeclass;
}

namespace resolution {
    // Definition_info forward declaration needed by mir::type::Structure,
    // mir::type::Enumeration, and mir::expression::Function_reference.

    template <class, class>
    struct Definition_info;

    using Function_info  = Definition_info<hir::definition::Function,  mir::Function>;
    using Struct_info    = Definition_info<hir::definition::Struct,    mir::Struct>;
    using Enum_info      = Definition_info<hir::definition::Enum,      mir::Enum>;
    using Alias_info     = Definition_info<hir::definition::Alias,     mir::Alias>;
    using Typeclass_info = Definition_info<hir::definition::Typeclass, mir::Typeclass>;
}


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
    Pattern           pattern;
    bu::Wrapper<Type> type;
};


DECLARE_FORMATTER_FOR(mir::Class_reference);

DECLARE_FORMATTER_FOR(mir::Function);
DECLARE_FORMATTER_FOR(mir::Struct);
DECLARE_FORMATTER_FOR(mir::Enum);
DECLARE_FORMATTER_FOR(mir::Alias);
DECLARE_FORMATTER_FOR(mir::Typeclass);