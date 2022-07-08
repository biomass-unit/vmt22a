#pragma once

#include "bu/utilities.hpp"
#include "ast/ast.hpp"


namespace hir {

    struct [[nodiscard]] Expression;

    struct [[nodiscard]] Function_argument;


    namespace expression {

        template <class T>
        struct Literal {
            T value;
            DEFAULTED_EQUALITY(Literal);
        };

        struct Array_literal {
            std::vector<Expression> elements;
            DEFAULTED_EQUALITY(Array_literal);
        };

        struct Tuple {
            std::vector<Expression> elements;
            DEFAULTED_EQUALITY(Tuple);
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

        struct Block {
            std::vector<Expression>                side_effects;
            std::optional<bu::Wrapper<Expression>> result;
            DEFAULTED_EQUALITY(Block);
        };

        struct Invocation {
            std::vector<Function_argument> arguments;
            bu::Wrapper<Expression>        invocable;
            DEFAULTED_EQUALITY(Invocation);
        };

        struct Match {
            struct Case {
                bu::Wrapper<ast::Pattern> pattern;
                bu::Wrapper<Expression>   expression;
                DEFAULTED_EQUALITY(Case);
            };
            std::vector<Case>       cases;
            bu::Wrapper<Expression> expression;
            DEFAULTED_EQUALITY(Match);
        };

    }

    struct Expression {
        using Variant = std::variant<
            expression::Literal<bu::Isize>,
            expression::Literal<bu::Float>,
            expression::Literal<bu::Char>,
            expression::Literal<bool>,
            expression::Literal<lexer::String>,
            expression::Array_literal,
            expression::Tuple,
            expression::Conditional,
            expression::Loop,
            expression::Break,
            expression::Block,
            expression::Invocation,
            expression::Match
        >;

        Variant                        value;
        std::optional<bu::Source_view> source_view;

        // Some HIR expressions are fabricated by the AST lowering
        // process, which means that they have no source view.
    };


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