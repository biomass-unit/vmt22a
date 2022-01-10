#pragma once

#include "bu/utilities.hpp"


namespace lexer {

    struct [[nodiscard]] Token {
        using Variant = std::variant<
            std::monostate,
            bu::Isize,
            bu::Float,
            bu::Char,
            bool
        >;

        enum class Type {
            dot,
            comma,
            colon,
            semicolon,
            double_colon,

            ellipsis,
            ampersand,
            pipe,

            paren_open,
            paren_close,
            brace_open,
            brace_close,
            bracket_open,
            bracket_close,

            let,
            mut,
            if_,
            else_,
            for_,
            in,
            while_,
            loop,
            continue_,
            break_,
            match,
            ret,
            fn,
            as,
            data,
            struct_,
            class_,
            inst,
            alias,
            import,
            size_of,
            type_of,
            meta,
            mod,

            underscore,
            lower_name,
            upper_name,
            operator_name,

            string,
            integer,
            floating,
            character,
            boolean,

            end_of_input
        };

        Variant value;
        Type type;

        static constexpr auto type_count = static_cast<bu::Usize>(Type::end_of_input) + 1;
    };

}