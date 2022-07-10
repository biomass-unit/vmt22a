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


    class Node_context {
        bu::Wrapper_context_for<Expression, ast::Pattern> wrapper_context;

        bu::Wrapper<Expression> unit_value = expression::Tuple {};
    public:
        explicit Node_context(bu::Wrapper_context<ast::Pattern>&& pattern_context)
            : wrapper_context { bu::Wrapper_context<Expression> {}, std::move(pattern_context) } {}

        auto get_unit_value() const noexcept -> bu::Wrapper<Expression> {
            return unit_value;
        }
    };


    struct Module {
        // todo
    };

}