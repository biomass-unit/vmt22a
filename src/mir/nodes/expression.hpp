#ifndef VMT22A_MIR_NODES_EXPRESSION
#define VMT22A_MIR_NODES_EXPRESSION
#else
#error This isn't supposed to be included by anything other than mir/mir.hpp
#endif


namespace mir {

    namespace expression {

        template <class T>
        using Literal = hir::expression::Literal<T>;

    }


    struct Expression {
        using Variant = std::variant<
            expression::Literal<bu::Isize>,
            expression::Literal<bu::Float>,
            expression::Literal<bu::Char>,
            expression::Literal<bool>
        >;

        Variant                        value;
        Type                           type;
        std::optional<bu::Source_view> source_view;
    };

}