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

        struct Constructor {
            Qualified_name                      name;
            std::optional<bu::Wrapper<Pattern>> pattern;
            DEFAULTED_EQUALITY(Constructor);
        };

        struct Constructor_shorthand {
            ::ast::Name                         name;
            std::optional<bu::Wrapper<Pattern>> pattern;
            DEFAULTED_EQUALITY(Constructor_shorthand);
        };

        struct Tuple {
            std::vector<Pattern> patterns;
            DEFAULTED_EQUALITY(Tuple);
        };

        struct Slice {
            std::vector<Pattern> patterns;
            DEFAULTED_EQUALITY(Slice);
        };

        struct As {
            Name                 name;
            bu::Wrapper<Pattern> pattern;
            DEFAULTED_EQUALITY(As);
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
            pattern::Constructor,
            pattern::Constructor_shorthand,
            pattern::Tuple,
            pattern::Slice,
            pattern::As,
            pattern::Guarded
        >;

        Variant         value;
        bu::Source_view source_view;
    };

}