#ifndef VMT22A_HIR_NODES_TYPE
#define VMT22A_HIR_NODES_TYPE
#else
#error This isn't supposed to be included by anything other than hir/hir.hpp
#endif


namespace hir {

    namespace type {

        using ast::type::Primitive;
        using ast::type::Integer;
        using ast::type::Floating;
        using ast::type::Character;
        using ast::type::Boolean;
        using ast::type::String;

        using ast::type::Wildcard;

        struct Typename {
            Qualified_name identifier;
            DEFAULTED_EQUALITY(Typename);
        };

        struct Implicit_parameter_reference {
            Implicit_template_parameter::Tag tag;
            DEFAULTED_EQUALITY(Implicit_parameter_reference);
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

        struct Slice {
            bu::Wrapper<Type> element_type;
            DEFAULTED_EQUALITY(Slice);
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

        struct Reference {
            bu::Wrapper<Type> type;
            ast::Mutability   mutability;
            DEFAULTED_EQUALITY(Reference);
        };

        struct Pointer {
            bu::Wrapper<Type> type;
            ast::Mutability   mutability;
            DEFAULTED_EQUALITY(Pointer);
        };

        struct Instance_of {
            std::vector<Class_reference> classes;
            DEFAULTED_EQUALITY(Instance_of);
        };

        struct Template_application {
            std::vector<Template_argument> arguments;
            Qualified_name                 name;
            DEFAULTED_EQUALITY(Template_application);
        };

        struct For_all {
            std::vector<Template_parameter> parameters;
            bu::Wrapper<Type>               body;
            DEFAULTED_EQUALITY(For_all);
        };

    }

    struct Type {
        using Variant = std::variant<
            type::Integer,
            type::Floating,
            type::Character,
            type::Boolean,
            type::String,
            type::Wildcard,
            type::Typename,
            type::Implicit_parameter_reference,
            type::Tuple,
            type::Array,
            type::Slice,
            type::Function,
            type::Type_of,
            type::Reference,
            type::Pointer,
            type::Instance_of,
            type::Template_application,
            type::For_all
        >;

        Variant         value;
        bu::Source_view source_view;

        DEFAULTED_EQUALITY(Type);
    };

}