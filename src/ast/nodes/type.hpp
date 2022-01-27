// Only included by ast/ast.hpp


namespace ast {

    namespace type {

        template <class>
        struct Primitive {
            DEFAULTED_EQUALITY(Primitive);
        };

        using Int    = Primitive<bu::Isize>;
        using Float  = Primitive<bu::Float>;
        using Char   = Primitive<bu::Char>;
        using Bool   = Primitive<bool>;
        using String = Primitive<lexer::String>;

        struct Typename {
            lexer::Identifier identifier;
            DEFAULTED_EQUALITY(Typename);
        };

        struct Tuple {
            std::vector<bu::Wrapper<Type>> types;
            DEFAULTED_EQUALITY(Tuple);
        };

        struct Array {
            bu::Wrapper<Type> element_type;
            bu::Usize length;
            DEFAULTED_EQUALITY(Array);
        };

        struct List {
            bu::Wrapper<Type> element_type;
            DEFAULTED_EQUALITY(List);
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
            type::Typename,
            type::Tuple,
            type::Array,
            type::List,
            type::Function,
            type::Type_of
        >;
        Variant value;

        DEFINE_NODE_CTOR(Type);
        DEFAULTED_EQUALITY(Type);
    };

}