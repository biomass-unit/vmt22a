// Only included by ast/ast.hpp


namespace ast {

    template <class T>
    struct Literal {
        T value;
        DEFAULTED_EQUALITY(Literal);
    };

    struct Array_literal {
        std::vector<Expression> elements;
        DEFAULTED_EQUALITY(Array_literal);
    };

    struct List_literal {
        std::vector<Expression> elements;
        DEFAULTED_EQUALITY(List_literal);
    };

    struct Variable {
        lexer::Identifier name;
        DEFAULTED_EQUALITY(Variable);
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
        std::vector<Expression> arguments;
        bu::Wrapper<Expression> invocable;
        DEFAULTED_EQUALITY(Invocation);
    };

    struct Binary_operator_invocation {
        bu::Wrapper<Expression> left;
        bu::Wrapper<Expression> right;
        lexer::Identifier       op;
        DEFAULTED_EQUALITY(Binary_operator_invocation);
    };

    struct Member_access {
        bu::Wrapper<Expression> expression;
        lexer::Identifier       member_name;
        DEFAULTED_EQUALITY(Member_access);
    };

    struct Member_function_invocation {
        std::vector<Expression> arguments;
        bu::Wrapper<Expression> expression;
        lexer::Identifier       member_name;
        DEFAULTED_EQUALITY(Member_function_invocation);
    };

    struct Tuple_member_access {
        bu::Wrapper<Expression> expression;
        bu::Isize               member_index;
        DEFAULTED_EQUALITY(Tuple_member_access);
    };

    struct Conditional {
        bu::Wrapper<Expression>                condition;
        bu::Wrapper<Expression>                true_branch;
        std::optional<bu::Wrapper<Expression>> false_branch;
        DEFAULTED_EQUALITY(Conditional);
    };

    struct Match {
        struct Case {
            bu::Wrapper<Pattern>    pattern;
            bu::Wrapper<Expression> expression;
            DEFAULTED_EQUALITY(Case);
        };
        std::vector<Case>       cases;
        bu::Wrapper<Expression> expression;
        DEFAULTED_EQUALITY(Match);
    };

    struct Type_cast {
        bu::Wrapper<Expression> expression;
        bu::Wrapper<Type>       target;
        DEFAULTED_EQUALITY(Type_cast);
    };

    struct Let_binding {
        bu::Wrapper<Pattern>             pattern;
        bu::Wrapper<Expression>          initializer;
        std::optional<bu::Wrapper<Type>> type;
        DEFAULTED_EQUALITY(Let_binding);
    };

    struct Infinite_loop {
        bu::Wrapper<Expression> body;
        DEFAULTED_EQUALITY(Infinite_loop);
    };

    struct While_loop {
        bu::Wrapper<Expression> condition;
        bu::Wrapper<Expression> body;
        DEFAULTED_EQUALITY(While_loop);
    };

    struct For_loop {
        bu::Wrapper<Pattern>    iterator;
        bu::Wrapper<Expression> iterable;
        bu::Wrapper<Expression> body;
        DEFAULTED_EQUALITY(For_loop);
    };

    struct Continue {
        DEFAULTED_EQUALITY(Continue);
    };

    struct Break {
        std::optional<bu::Wrapper<Expression>> expression;
        DEFAULTED_EQUALITY(Break);
    };

    struct Ret {
        std::optional<bu::Wrapper<Expression>> expression;
        DEFAULTED_EQUALITY(Ret);
    };

    struct Size_of {
        bu::Wrapper<Type> type;
        DEFAULTED_EQUALITY(Size_of);
    };

    struct Take_reference {
        bu::Wrapper<Expression> expression;
        bool is_mutable;
        DEFAULTED_EQUALITY(Take_reference);
    };

    struct Meta {
        bu::Wrapper<Expression> expression;
        DEFAULTED_EQUALITY(Meta);
    };


    struct Expression {
        using Variant = std::variant<
            Literal<bu::Isize>,
            Literal<bu::Float>,
            Literal<char>,
            Literal<bool>,
            Literal<lexer::String>,
            Array_literal,
            List_literal,
            lexer::Identifier,
            Variable,
            Tuple,
            Compound_expression,
            Invocation,
            Binary_operator_invocation,
            Member_access,
            Member_function_invocation,
            Tuple_member_access,
            Conditional,
            Match,
            Type_cast,
            Let_binding,
            Infinite_loop,
            While_loop,
            For_loop,
            Continue,
            Break,
            Ret,
            Size_of,
            Take_reference,
            Meta
        >;
        Variant          value;
        std::string_view source_view;

        DEFINE_NODE_CTOR(Expression);
        DEFAULTED_EQUALITY(Expression);
    };

    static_assert(std::is_trivially_copyable_v<bu::Wrapper<Expression>>);

}