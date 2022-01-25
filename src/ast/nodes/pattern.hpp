// Only included by ast/ast.hpp


namespace ast {

    namespace pattern {

        struct Wildcard {
            DEFAULTED_EQUALITY(Wildcard);
        };

        struct Name {
            lexer::Identifier identifier;
            DEFAULTED_EQUALITY(Name);
        };

        struct Tuple {
            std::vector<Pattern> patterns;
            DEFAULTED_EQUALITY(Tuple);
        };

        template <class T>
        using Literal = ::ast::Literal<T>;

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
            pattern::Tuple
        >;
        Variant value;

        DEFINE_NODE_CTOR(Pattern);
        DEFAULTED_EQUALITY(Pattern);
    };

}