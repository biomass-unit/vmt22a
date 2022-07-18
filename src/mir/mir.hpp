#pragma once

#include "bu/utilities.hpp"
#include "bu/source.hpp"


namespace mir {

    struct [[nodiscard]] Expression;
    struct [[nodiscard]] Pattern;
    struct [[nodiscard]] Type;


    namespace type {

        enum class Integer {
            i8, i16, i32, i64,
            u8, u16, u32, u64,
        };

        struct Floating  {};
        struct Character {};
        struct Boolean   {};

    }

    struct Type {
        using Variant = std::variant<
            type::Integer,
            type::Floating,
            type::Boolean
        >;
        std::optional<bu::Source_view> source_view;
    };


    namespace expression {

        template <class T>
        struct Literal {

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

}