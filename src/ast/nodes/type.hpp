// Only included by ast/ast.hpp


namespace ast {

    namespace type {

        template <class>
        struct Primitive {};

        using Int    = Primitive<bu::Isize>;
        using Float  = Primitive<bu::Float>;
        using Char   = Primitive<bu::Char>;
        using Bool   = Primitive<bool>;
        using String = Primitive<lexer::String_literal>;

        struct Tuple {
            std::vector<bu::Wrapper<Type>> types;
            DEFAULTED_EQUALITY(Tuple);
        };

        struct Function {
            std::vector<bu::Wrapper<Type>> argument_types;
            bu::Wrapper<Type> return_type;
            DEFAULTED_EQUALITY(Function);
        };

    }


    struct Type {
        using Variant = std::variant<
            type::Int,
            type::Float,
            type::Char,
            type::Bool,
            type::Tuple,
            type::Function
        >;
        Variant value;
        DEFAULTED_EQUALITY(Type);
    };

}