// Only included by ast/ast.hpp


namespace ast {

    namespace type {

        template <class>
        struct Primitive {
            DEFAULTED_EQUALITY(Primitive);
        };

        using Integer   = Primitive<bu::Isize>;
        using Floating  = Primitive<bu::Float>;
        using Character = Primitive<bu::Char>;
        using Boolean   = Primitive<bool>;
        using String    = Primitive<lexer::String>;

        struct Typename {
            lexer::Identifier identifier;
            DEFAULTED_EQUALITY(Typename);
        };

        struct Tuple {
            std::vector<Type> types;
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
            std::vector<Type> argument_types;
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
            type::Integer,
            type::Floating,
            type::Character,
            type::Boolean,
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


    namespace type {

        inline Type const
            integer   = Integer   {},
            floating  = Floating  {},
            character = Character {},
            boolean   = Boolean   {},
            string    = String    {};

    }

}