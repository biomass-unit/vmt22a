// Only included by ast/ast.hpp


namespace ast {

    template <class T>
    struct Literal {
        T value;
        DEFAULTED_EQUALITY(Literal);
    };

    struct Tuple {
        std::vector<Expression> expressions;
        DEFAULTED_EQUALITY(Tuple);
    };

    struct Compound_expression {
        std::vector<Expression> expressions;
        DEFAULTED_EQUALITY(Compound_expression);
    };

    struct Invocation {
        bu::Wrapper<Expression> invocable;
        std::vector<Expression> arguments;
        DEFAULTED_EQUALITY(Invocation);
    };

    struct Conditional {
        bu::Wrapper<Expression> condition, true_branch, false_branch;
        DEFAULTED_EQUALITY(Conditional);
    };

    struct Match {
        struct Case {
            bu::Wrapper<Pattern> pattern;
            bu::Wrapper<Expression> expression;
            DEFAULTED_EQUALITY(Case);
        };
        std::vector<Case> cases;
        bu::Wrapper<Expression> expression;
        DEFAULTED_EQUALITY(Match);
    };

    struct Type_cast {
        bu::Wrapper<Expression> expression;
        bu::Wrapper<Type> target;
        DEFAULTED_EQUALITY(Type_cast);
    };

    struct Let_binding {
        bu::Wrapper<Pattern> pattern;
        bu::Wrapper<Expression> initializer;
        std::optional<bu::Wrapper<Type>> type;
        DEFAULTED_EQUALITY(Let_binding);
    };


    struct Expression {
        using Variant = std::variant<
            Literal<bu::Isize>,
            Literal<bu::Float>,
            Literal<bu::Char>,
            Literal<bool>,
            Tuple,
            Compound_expression,
            Invocation,
            Conditional,
            Match,
            Type_cast,
            Let_binding
        >;
        Variant value;
        DEFAULTED_EQUALITY(Expression);
    };

}