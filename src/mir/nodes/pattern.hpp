#ifndef VMT22A_MIR_NODES_PATTERN
#define VMT22A_MIR_NODES_PATTERN
#else
#error This isn't supposed to be included by anything other than mir/mir.hpp
#endif


namespace mir {

    namespace pattern {

        struct Wildcard {};

        struct Name {
            ast::Name value;
            bool      is_mutable;
        };

        struct Tuple {
            std::vector<Pattern> patterns;
        };

        struct Slice {
            std::vector<Pattern> patterns;
        };

        struct Enum_constructor {
            std::optional<bu::Wrapper<Pattern>> pattern;
            Enum::Constructor                   constructor;
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
            pattern::Wildcard,
            pattern::Name,
            pattern::Tuple,
            pattern::Slice,
            pattern::Enum_constructor,
            pattern::As,
            pattern::Guarded
        >;

        Variant         value;
        bu::Source_view source_view;
    };

}