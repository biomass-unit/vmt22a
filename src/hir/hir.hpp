#pragma once

#include "bu/utilities.hpp"
#include "ast/ast.hpp"


namespace hir {

    struct [[nodiscard]] Expression;


    namespace expression {

        template <class T>
        struct Literal {
            T value;
            DEFAULTED_EQUALITY(Literal);
        };

        struct Conditional {
            bu::Wrapper<Expression> condition;
            bu::Wrapper<Expression> true_branch;
            bu::Wrapper<Expression> false_branch;
            DEFAULTED_EQUALITY(Conditional);
        };

        struct Loop {
            bu::Wrapper<Expression> body;
            DEFAULTED_EQUALITY(Loop);
        };

        struct Break {
            DEFAULTED_EQUALITY(Break);
        };

    }

    struct Expression {
        using Variant = std::variant<
            expression::Literal<bu::Isize>,
            expression::Literal<bu::Float>,
            expression::Literal<bu::Char>,
            expression::Literal<bool>,
            expression::Literal<lexer::String>,
            expression::Conditional,
            expression::Loop,
            expression::Break
        >;

        Variant         value;
        bu::Source_view source_view;
    };


    struct Module {
        // todo
    };

}