#ifndef VMT22A_MIR_NODES_TYPE
#define VMT22A_MIR_NODES_TYPE
#else
#error This isn't supposed to be included by anything other than mir/mir.hpp
#endif


namespace mir {

    struct Type_variable_tag {
        bu::Usize value;
    };

    namespace type {

        enum class Integer {
            i8, i16, i32, i64,
            u8, u16, u32, u64,
        };

        struct Floating  {};
        struct Character {};
        struct Boolean   {};
        struct String    {};

        struct Tuple {
            std::vector<bu::Wrapper<Type>> types;
        };

        struct Array {
            bu::Wrapper<Type>       element_type;
            bu::Wrapper<Expression> length;
        };

        struct Slice {
            bu::Wrapper<Type> element_type;
        };

        struct Function {
            std::vector<bu::Wrapper<Type>> parameter_types;
            bu::Wrapper<Type>              return_type;
        };

        struct Reference {
            ast::Mutability   mutability;
            bu::Wrapper<Type> referenced_type;
        };

        struct Parameterized {
            Template_parameter_set parameters;
            bu::Wrapper<Type>      body;
        };

        struct Structure {
            bu::Wrapper<resolution::Struct_info> info;
        };

        struct Enumeration {
            bu::Wrapper<resolution::Enum_info> info;
        };

        struct General_variable {
            Type_variable_tag tag;
        };

        struct Integral_variable {
            Type_variable_tag tag;
        };

    }


    struct Type {
        using Variant = std::variant<
            type::Tuple,
            type::Integer,
            type::Floating,
            type::Character,
            type::Boolean,
            type::String,
            type::Array,
            type::Slice,
            type::Function,
            type::Reference,
            type::Parameterized,
            type::Structure,
            type::Enumeration,
            type::General_variable,
            type::Integral_variable
        >;
        Variant                        value;
        std::optional<bu::Source_view> source_view;
    };

}