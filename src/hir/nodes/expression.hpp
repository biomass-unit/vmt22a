#ifndef VMT22A_HIR_NODES_EXPRESSION
#define VMT22A_HIR_NODES_EXPRESSION
#else
#error This isn't supposed to be included by anything other than hir/hir.hpp
#endif


namespace hir {

    namespace expression {

        template <class T>
        using Literal = ast::expression::Literal<T>;

        struct Array_literal {
            std::vector<Expression> elements;
            DEFAULTED_EQUALITY(Array_literal);
        };

        struct Variable {
            Qualified_name name;
            DEFAULTED_EQUALITY(Variable);
        };

        struct Tuple {
            std::vector<Expression> elements;
            DEFAULTED_EQUALITY(Tuple);
        };

        struct Loop {
            bu::Wrapper<Expression> body;
            DEFAULTED_EQUALITY(Loop);
        };

        struct Continue {
            DEFAULTED_EQUALITY(Continue);
        };

        struct Break {
            std::optional<ast::Name>               label;
            std::optional<bu::Wrapper<Expression>> expression;
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

        struct Struct_initializer {
            bu::Flatmap<lexer::Identifier, Expression> member_initializers;
            bu::Wrapper<Type>                          type;
            DEFAULTED_EQUALITY(Struct_initializer);
        };

        struct Binary_operator_invocation {
            bu::Wrapper<Expression> left;
            bu::Wrapper<Expression> right;
            lexer::Identifier       op;
            DEFAULTED_EQUALITY(Binary_operator_invocation);
        };

        struct Member_access_chain {
            struct Tuple_field {
                bu::Isize index;
                DEFAULTED_EQUALITY(Tuple_field);
            };
            struct Struct_field {
                ast::Name name;
                DEFAULTED_EQUALITY(Struct_field);
            };
            struct Array_index {
                bu::Wrapper<Expression> expression;
                DEFAULTED_EQUALITY(Array_index);
            };
            using Accessor = std::variant<Tuple_field, Struct_field, Array_index>;
            std::vector<Accessor>   accessors;
            bu::Wrapper<Expression> expression;
            DEFAULTED_EQUALITY(Member_access_chain);
        };

        struct Member_function_invocation {
            std::vector<Function_argument> arguments;
            bu::Wrapper<Expression>        expression;
            ast::Name                      member_name;
            DEFAULTED_EQUALITY(Member_function_invocation);
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

        struct Dereference {
            bu::Wrapper<Expression> expression;
            DEFAULTED_EQUALITY(Dereference);
        };

        struct Template_application {
            std::vector<Template_argument> template_arguments;
            Qualified_name                 name;
            DEFAULTED_EQUALITY(Template_application);
        };

        struct Type_cast {
            bu::Wrapper<Expression>          expression;
            bu::Wrapper<Type>                target;
            ast::expression::Type_cast::Kind kind;
            DEFAULTED_EQUALITY(Type_cast);
        };

        struct Let_binding {
            bu::Wrapper<Pattern>             pattern;
            bu::Wrapper<Expression>          initializer;
            std::optional<bu::Wrapper<Type>> type;
            DEFAULTED_EQUALITY(Let_binding);
        };

        struct Local_type_alias {
            lexer::Identifier name;
            bu::Wrapper<Type> type;
            DEFAULTED_EQUALITY(Local_type_alias);
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
            ast::Mutability   mutability;
            lexer::Identifier name;
            DEFAULTED_EQUALITY(Take_reference);
        };

        struct Placement_init {
            bu::Wrapper<Expression> lvalue;
            bu::Wrapper<Expression> initializer;
            DEFAULTED_EQUALITY(Placement_init);
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
            expression::Tuple,
            expression::Loop,
            expression::Break,
            expression::Continue,
            expression::Block,
            expression::Invocation,
            expression::Struct_initializer,
            expression::Binary_operator_invocation,
            expression::Member_access_chain,
            expression::Member_function_invocation,
            expression::Match,
            expression::Dereference,
            expression::Template_application,
            expression::Type_cast,
            expression::Let_binding,
            expression::Local_type_alias,
            expression::Ret,
            expression::Size_of,
            expression::Take_reference,
            expression::Placement_init,
            expression::Meta,
            expression::Hole
        >;

        Variant         value;
        bu::Source_view source_view;

        DEFAULTED_EQUALITY(Expression);
    };

}