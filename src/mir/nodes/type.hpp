#ifndef VMT22A_MIR_NODES_TYPE
#define VMT22A_MIR_NODES_TYPE
#else
#error This isn't supposed to be included by anything other than mir/mir.hpp
#endif


namespace mir {

    namespace type {

        enum class Integer {
            i8, i16, i32, i64,
            u8, u16, u32, u64,
        };

        struct Floating  {};
        struct Character {};
        struct Boolean   {};

        struct Tuple {
            std::vector<Type> types;
        };

        struct Function {
            std::vector<Type> arguments;
            bu::Wrapper<Type> return_type;
        };

        struct Reference {
            ast::Mutability   mutability;
            bu::Wrapper<Type> referenced_type;
        };

        struct Parameterized {
            Template_parameter_set parameters;
            bu::Wrapper<Type>      body;
        };

        struct Variable {
            struct Tag {
                bu::Usize value;
            };

            enum class Kind {
                any_integer,    // Arises from an integer literal with no explicit sign. Can be unified with any sufficiently large integer type.
                signed_integer, // Arises from a negative integer literal. Can be unified with any sufficiently large signed integer type.
                floating,       // Arises from a floating point literal.
                general,        // Arises from any other expression.
            };

            Tag  tag;
            Kind kind;
        };

    }


    struct Type {
        using Variant = std::variant<
            type::Integer,
            type::Floating,
            type::Boolean,
            type::Tuple,
            type::Function,
            type::Reference,
            type::Parameterized,
            type::Variable
        >;
        Variant                        value;
        std::optional<bu::Source_view> source_view;
    };

}