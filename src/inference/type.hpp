#pragma once

#include "bu/utilities.hpp"
#include "bu/wrapper.hpp"


namespace inference {

    struct [[nodiscard]] Type;


    namespace type {

        struct Boolean {};

        enum class Integer {
            i8, i16, i32, i64,
            u8, u16, u32, u64,

            _integer_count
        };

        enum class Floating {
            f32, f64,
        };

        struct Tuple {
            std::vector<Type> types;
        };

        struct Function {
            std::vector<Type> arguments;
            bu::Wrapper<Type> return_type;
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


    struct Type : std::variant<
        type::Boolean,
        type::Integer,
        type::Floating,
        type::Tuple,
        type::Function,
        type::Variable
    > {};


    auto unit_type() noexcept -> Type;

}

DECLARE_FORMATTER_FOR(inference::Type);