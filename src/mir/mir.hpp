#pragma once

#include "bu/utilities.hpp"
#include "bu/source.hpp"


namespace mir {

    struct [[nodiscard]] Expression;
    struct [[nodiscard]] Pattern;
    struct [[nodiscard]] Type;


    struct Template_parameter {};


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
            std::vector<Template_parameter> parameters;
            bu::Wrapper<Type>               body;
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


    namespace expression {

        template <class T>
        struct Literal {
            T value;
        };

    }
    
    struct Expression {
        using Variant = std::variant<
            expression::Literal<bu::I8>,
            expression::Literal<bu::I16>,
            expression::Literal<bu::I32>,
            expression::Literal<bu::I64>,
            expression::Literal<bu::U8>,
            expression::Literal<bu::U16>,
            expression::Literal<bu::U32>,
            expression::Literal<bu::U64>,
            expression::Literal<bu::Float>,
            expression::Literal<bu::Char>,
            expression::Literal<bool>
        >;

        Variant                        value;
        Type                           type;
        std::optional<bu::Source_view> source_view;
    };


    namespace pattern {

        struct Wildcard {};

    }

    struct Pattern {
        using Variant = std::variant<
            pattern::Wildcard
        >;

        Variant                        value;
        std::optional<bu::Source_view> source_view;
    };


    using Node_context = bu::Wrapper_context_for<Expression, Pattern, Type>;

}