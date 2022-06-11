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

        struct Variable {
            Qualified_name name;
            DEFAULTED_EQUALITY(Variable);
        };

        struct Dereference {
            bu::Wrapper<Expression> expression;
            DEFAULTED_EQUALITY(Dereference);
        };

        struct Template_instantiation {
            std::vector<Template_argument> template_arguments;
            Qualified_name                 name;
            DEFAULTED_EQUALITY(Template_instantiation);
        };

        struct Tuple {
            std::vector<Expression> expressions;
            DEFAULTED_EQUALITY(Tuple);
        };

        struct Compound {
            std::vector<Expression> expressions;
            DEFAULTED_EQUALITY(Compound);
        };

        struct Invocation {
            std::vector<Function_argument> arguments;
            bu::Wrapper<Expression>        invocable;
            DEFAULTED_EQUALITY(Invocation);
        };

        struct Struct_initializer {
            bu::Flatmap<lexer::Identifier, bu::Wrapper<Expression>> member_initializers;
            bu::Wrapper<Type>                                       type;
            DEFAULTED_EQUALITY(Struct_initializer);
        };

        struct Binary_operator_invocation {
            bu::Wrapper<Expression> left;
            bu::Wrapper<Expression> right;
            lexer::Identifier       op;
            DEFAULTED_EQUALITY(Binary_operator_invocation);
        };

        struct Member_access_chain {
            using Accessor = std::variant<lexer::Identifier, bu::Isize>;
            std::vector<Accessor>   accessors;
            bu::Wrapper<Expression> expression;
            DEFAULTED_EQUALITY(Member_access_chain);
        };

        struct Member_function_invocation {
            std::vector<Function_argument> arguments;
            bu::Wrapper<Expression>        expression;
            lexer::Identifier              member_name;
            DEFAULTED_EQUALITY(Member_function_invocation);
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
            enum class Kind {
                conversion,
                ascription,
            };
            bu::Wrapper<Expression> expression;
            bu::Wrapper<Type>       target;
            Kind                    kind = Kind::conversion;
            DEFAULTED_EQUALITY(Type_cast);
        };

        struct Let_binding {
            bu::Wrapper<Pattern>             pattern;
            bu::Wrapper<Expression>          initializer;
            std::optional<bu::Wrapper<Type>> type;
            DEFAULTED_EQUALITY(Let_binding);
        };

        struct Conditional_let {
            bu::Wrapper<Pattern>    pattern;
            bu::Wrapper<Expression> initializer;
            DEFAULTED_EQUALITY(Conditional_let);
        };

        struct Local_type_alias {
            lexer::Identifier name;
            bu::Wrapper<Type> type;
            DEFAULTED_EQUALITY(Local_type_alias);
        };

        struct Lambda {
            struct Capture {
                struct By_pattern {
                    bu::Wrapper<Pattern>    pattern;
                    bu::Wrapper<Expression> expression;
                    DEFAULTED_EQUALITY(By_pattern);
                };
                struct By_reference {
                    lexer::Identifier variable;
                    DEFAULTED_EQUALITY(By_reference);
                };
                using Variant = std::variant<By_pattern, By_reference>;

                Variant         value;
                bu::Source_view source_view;
                DEFAULTED_EQUALITY(Capture);
            };
            bu::Wrapper<Expression>         body;
            std::vector<Function_parameter> parameters;
            std::vector<Capture>            explicit_captures;
            DEFAULTED_EQUALITY(Lambda);
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
            Mutability        mutability;
            lexer::Identifier name;
            DEFAULTED_EQUALITY(Take_reference);
        };

        struct Meta {
            bu::Wrapper<Expression> expression;
            DEFAULTED_EQUALITY(Meta);
        };

        struct Hole {
            DEFAULTED_EQUALITY(Hole);
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
            expression::Variable,
            expression::Dereference,
            expression::Template_instantiation,
            expression::Tuple,
            expression::Compound,
            expression::Invocation,
            expression::Struct_initializer,
            expression::Binary_operator_invocation,
            expression::Member_access_chain,
            expression::Member_function_invocation,
            expression::Conditional,
            expression::Match,
            expression::Type_cast,
            expression::Let_binding,
            expression::Conditional_let,
            expression::Local_type_alias,
            expression::Lambda,
            expression::Infinite_loop,
            expression::While_loop,
            expression::For_loop,
            expression::Continue,
            expression::Break,
            expression::Ret,
            expression::Size_of,
            expression::Take_reference,
            expression::Meta,
            expression::Hole
        >;

        Variant         value;
        bu::Source_view source_view;
    };

    static_assert(std::is_trivially_copyable_v<bu::Wrapper<Expression>>);

}