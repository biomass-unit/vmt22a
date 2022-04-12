// Only included by ast/ast.hpp


namespace ast {

    namespace pattern {

        struct Wildcard {
            DEFAULTED_EQUALITY(Wildcard);
        };

        struct Name {
            lexer::Identifier identifier;
            Mutability        mutability;
            DEFAULTED_EQUALITY(Name);
        };

        struct Tuple {
            std::vector<Pattern> patterns;
            DEFAULTED_EQUALITY(Tuple);
        };

        struct Guarded {
            bu::Wrapper<Pattern> pattern;
            bu::Wrapper<Expression> guard;
            DEFAULTED_EQUALITY(Guarded);
        };

        template <class T>
        using Literal = expression::Literal<T>;

    }


    struct Pattern {
        using Variant = std::variant<
            pattern::Wildcard,
            pattern::Literal<bu::Isize>,
            pattern::Literal<bu::Float>,
            pattern::Literal<char>,
            pattern::Literal<bool>,
            pattern::Literal<lexer::String>,
            pattern::Name,
            pattern::Tuple,
            pattern::Guarded
        >;

        Variant         value;
        bu::Source_view source_view;
    };

}