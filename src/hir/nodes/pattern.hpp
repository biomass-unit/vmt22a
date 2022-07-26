#ifndef VMT22A_HIR_NODES_PATTERN
#define VMT22A_HIR_NODES_PATTERN
#else
#error This isn't supposed to be included by anything other than hir/hir.hpp
#endif


namespace hir {

    namespace pattern {

        using ast::pattern::Literal;
        using ast::pattern::Wildcard;
        using ast::pattern::Name;

        struct Constructor {
            Qualified_name                      name;
            std::optional<bu::Wrapper<Pattern>> pattern;
            DEFAULTED_EQUALITY(Constructor);
        };

        struct Constructor_shorthand {
            ast::Name                           name;
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
            Expression           guard;
            DEFAULTED_EQUALITY(Guarded);
        };

    }

    struct Pattern {
        using Variant = std::variant<
            pattern::Literal<bu::Isize>,
            pattern::Literal<bu::Float>,
            pattern::Literal<bu::Char>,
            pattern::Literal<bool>,
            pattern::Literal<lexer::String>,
            pattern::Wildcard,
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

        DEFAULTED_EQUALITY(Pattern);
    };

}