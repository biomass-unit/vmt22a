// Only included by ast/ast.hpp


namespace ast {

    namespace pattern {

        struct Wildcard {
            DEFAULTED_EQUALITY(Wildcard);
        };

    }


    struct Pattern {
        using Variant = std::variant<
            pattern::Wildcard
        >;
        Variant value;
        DEFAULTED_EQUALITY(Pattern);
    };

}