#ifndef VMT22A_MIR_NODES_EXPRESSION
#define VMT22A_MIR_NODES_EXPRESSION
#else
#error This isn't supposed to be included by anything other than mir/mir.hpp
#endif


namespace mir {

    namespace expression {

        template <class T>
        using Literal = hir::expression::Literal<T>;

        struct Array_literal {
            std::vector<Expression> elements;
        };

        struct Tuple {
            std::vector<Expression> elements;
        };

        struct Block {
            std::vector<Expression>                side_effects;
            std::optional<bu::Wrapper<Expression>> result;
        };

        struct Let_binding {
            bu::Wrapper<Pattern>    pattern;
            bu::Wrapper<Type>       type;
            bu::Wrapper<Expression> initializer;
        };

        struct Local_variable_reference {
            ast::Name name;
        };

        struct Function_reference {
            bu::Wrapper<resolution::Function_info> info;
        };

        struct Direct_invocation {
            Function_reference      function;
            std::vector<Expression> arguments;
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
            expression::Block,
            expression::Let_binding,
            expression::Local_variable_reference,
            expression::Function_reference,
            expression::Direct_invocation
        >;

        Variant           value;
        bu::Wrapper<Type> type;
        bu::Source_view   source_view;
    };

}