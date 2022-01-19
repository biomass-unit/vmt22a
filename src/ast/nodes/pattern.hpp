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

        template <class X>
        Pattern(X&& x) noexcept(std::is_nothrow_constructible_v<Variant, X&&>)
            : value { std::forward<X>(x) } {}

        DEFAULTED_EQUALITY(Pattern);
    };

}