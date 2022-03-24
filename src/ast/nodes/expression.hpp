// Only included by ast/ast.hpp


namespace ast {

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

        struct List_literal {
            std::vector<Expression> elements;
            DEFAULTED_EQUALITY(List_literal);
        };

        struct Variable {
            Qualified_name name;
            DEFAULTED_EQUALITY(Variable);
        };

        struct Template_instantiation {
            Qualified_name                 name;
            std::vector<Template_argument> template_arguments;
            DEFAULTED_EQUALITY(Template_instantiation);
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
            std::vector<Function_argument> arguments;
            bu::Wrapper<Expression>        invocable;
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
            std::vector<Function_argument> arguments;
            bu::Wrapper<Expression>        expression;
            lexer::Identifier              member_name;
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
            Mutability              mutability;
            DEFAULTED_EQUALITY(Take_reference);
        };

        struct Meta {
            bu::Wrapper<Expression> expression;
            DEFAULTED_EQUALITY(Meta);
        };

    }


    struct Expression : dtl::Source_tracked {
        using Variant = std::variant<
            expression::Literal<bu::Isize>,
            expression::Literal<bu::Float>,
            expression::Literal<bu::Char>,
            expression::Literal<bool>,
            expression::Literal<lexer::String>,
            expression::Array_literal,
            expression::List_literal,
            expression::Variable,
            expression::Template_instantiation,
            expression::Tuple,
            expression::Compound_expression,
            expression::Invocation,
            expression::Binary_operator_invocation,
            expression::Member_access,
            expression::Member_function_invocation,
            expression::Tuple_member_access,
            expression::Conditional,
            expression::Match,
            expression::Type_cast,
            expression::Let_binding,
            expression::Infinite_loop,
            expression::While_loop,
            expression::For_loop,
            expression::Continue,
            expression::Break,
            expression::Ret,
            expression::Size_of,
            expression::Take_reference,
            expression::Meta
        >;

        Variant value;

        DEFINE_NODE_CTOR(Expression);
    };

    static_assert(std::is_trivially_copyable_v<bu::Wrapper<Expression>>);

}