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

        template <class X>
        Pattern(X&& x) noexcept(std::is_nothrow_constructible_v<Variant, X&&>)
            : value { std::forward<X>(x) } {}

        DEFAULTED_EQUALITY(Pattern);
    };

}