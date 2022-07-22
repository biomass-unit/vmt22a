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

    }


    struct Expression {
        using Variant = std::variant<
            expression::Literal<bu::Isize>,
            expression::Literal<bu::Float>,
            expression::Literal<bu::Char>,
            expression::Literal<bool>,
            expression::Literal<lexer::String>,
            expression::Array_literal,
            expression::Tuple
        >;

        Variant                        value;
        bu::Wrapper<Type>              type;
        std::optional<bu::Source_view> source_view;
    };

}