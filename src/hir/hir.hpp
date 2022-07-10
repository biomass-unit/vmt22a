#pragma once

#include "bu/utilities.hpp"
#include "ast/ast.hpp"


namespace hir {

    struct [[nodiscard]] Expression;
    struct [[nodiscard]] Type;
    struct [[nodiscard]] Pattern;
    struct [[nodiscard]] Definition;

    struct [[nodiscard]] Function_argument;


    struct Function_parameter {

    };

    struct Template_parameter {

    };

}


#include "nodes/expression.hpp"
#include "nodes/pattern.hpp"
#include "nodes/type.hpp"
#include "nodes/definition.hpp"


namespace hir {

    struct Function_argument {
        Expression               expression;
        std::optional<ast::Name> name;
        DEFAULTED_EQUALITY(Function_argument);
    };


    struct Node_context {
        bu::Wrapper_context_for<
            Expression,
            Type,
            Pattern,
            Definition
        > wrapper_context;

        bu::Wrapper<Expression> unit_value = expression::Tuple {};
        bu::Wrapper<Type>       unit_type  = type      ::Tuple {};
    };


    struct Module {
        Node_context            node_context;
        bu::Source              source;
        std::vector<Definition> definitions;
    };

}