#pragma once

#include "bu/utilities.hpp"
#include "bu/bounded_integer.hpp"


namespace inference {

    struct [[nodiscard]] Type;


    namespace type {

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

        struct Variable {
            struct Tag {
                bu::Usize value;
            };

            enum class Kind {
                general,
                integer,
                floating
            };

            Tag  tag;
            Kind kind;
        };

    }


    struct Type {
        using Variant = std::variant<
            type::Integer,
            type::Floating,
            type::Tuple,
            type::Variable
        >;

        Variant value;
    };


    auto unit_type() noexcept -> Type::Variant;


    class Context {
        bu::Bounded_usize current_type_variable_tag;
    public:
        auto fresh_type_variable(type::Variable::Kind = type::Variable::Kind::general) -> type::Variable;
    };

}


DECLARE_FORMATTER_FOR(inference::Type);