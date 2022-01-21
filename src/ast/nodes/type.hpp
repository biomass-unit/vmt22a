// Only included by ast/ast.hpp


namespace ast {

    namespace type {

        template <class>
        struct Primitive {};

        using Int    = Primitive<bu::Isize>;
        using Float  = Primitive<bu::Float>;
        using Char   = Primitive<bu::Char>;
        using Bool   = Primitive<bool>;
        using String = Primitive<lexer::String>;

        struct Tuple {
            std::vector<bu::Wrapper<Type>> types;
            DEFAULTED_EQUALITY(Tuple);
        };

        struct Function {
            std::vector<bu::Wrapper<Type>> argument_types;
            bu::Wrapper<Type> return_type;
            DEFAULTED_EQUALITY(Function);
        };

        struct Type_of {
            bu::Wrapper<Expression> expression;
            DEFAULTED_EQUALITY(Type_of);
        };

    }


    struct Type {
        using Variant = std::variant<
            type::Int,
            type::Float,
            type::Char,
            type::Bool,
            type::String,
            type::Tuple,
            type::Function,
            type::Type_of
        >;
        Variant value;

        template <class X>
        Type(X&& x) noexcept(std::is_nothrow_constructible_v<Variant, X&&>)
            : value { std::forward<X>(x) } {}

        DEFAULTED_EQUALITY(Type);
    };

}