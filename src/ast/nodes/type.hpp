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
            Qualified_name identifier;
            DEFAULTED_EQUALITY(Typename);
        };

        struct Tuple {
            std::vector<Type> types;
            DEFAULTED_EQUALITY(Tuple);
        };

        struct Array {
            bu::Wrapper<Type>       element_type;
            bu::Wrapper<Expression> length;
            DEFAULTED_EQUALITY(Array);
        };

        struct List {
            bu::Wrapper<Type> element_type;
            DEFAULTED_EQUALITY(List);
        };

        struct Function {
            std::vector<bu::Wrapper<Type>> argument_types;
            bu::Wrapper<Type>              return_type;
            DEFAULTED_EQUALITY(Function);
        };

        struct Type_of {
            bu::Wrapper<Expression> expression;
            DEFAULTED_EQUALITY(Type_of);
        };

        struct Reference {
            bu::Wrapper<Type> type;
            Mutability        mutability;
            DEFAULTED_EQUALITY(Reference);
        };

        struct Pointer {
            bu::Wrapper<Type> type;
            Mutability        mutability;
            DEFAULTED_EQUALITY(Pointer);
        };

        struct Template_instantiation {
            std::vector<ast::Template_argument> arguments;
            Qualified_name                      name;
            DEFAULTED_EQUALITY(Template_instantiation);
        };

        struct Inference_variable {
            bu::Usize tag;
            DEFAULTED_EQUALITY(Inference_variable);
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
            type::Type_of,
            type::Reference,
            type::Pointer,
            type::Template_instantiation,
            type::Inference_variable
        >;
        Variant value;

        DEFINE_NODE_CTOR(Type);

        auto operator==(Type const& other) const noexcept -> bool {
            return value == other.value;
        }
    };


    namespace type {

        inline bu::Wrapper<Type> const
            integer   = Integer   {},
            floating  = Floating  {},
            character = Character {},
            boolean   = Boolean   {},
            string    = String    {},
            unit      = Tuple     {};

    }

}