#pragma once

#include "bu/utilities.hpp"
#include "bu/pooled_string.hpp"
#include "bu/textual_error.hpp"


namespace lexer {

    using String     = bu::Pooled_string<struct _string_tag>;
    using Identifier = bu::Pooled_string<struct _identifier_tag>;


    struct [[nodiscard]] Token {
        using Variant = std::variant<
            std::monostate,
            bu::Isize,
            bu::Float,
            char,
            bool,
            String,
            Identifier
        >;

        enum class Type {
            dot,
            comma,
            colon,
            semicolon,
            double_colon,

            ampersand,
            asterisk,
            plus,
            question,
            equals,
            pipe,
            right_arrow,

            paren_open,
            paren_close,
            brace_open,
            brace_close,
            bracket_open,
            bracket_close,

            let,
            mut,
            immut,
            if_,
            else_,
            elif,
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
            impl,
            alias,
            namespace_,
            import_,
            export_,
            module_,
            size_of,
            type_of,
            meta,
            where,
            dyn,

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

        Variant          value;
        Type             type;
        std::string_view source_view;

        static constexpr auto type_count = static_cast<bu::Usize>(Type::end_of_input) + 1;

        template <class T>
        inline auto value_as() noexcept -> T& {
            assert(std::holds_alternative<T>(value));
            return *std::get_if<T>(&value);
        }

        inline auto& as_integer    () noexcept { return value_as<bu::Isize >(); }
        inline auto& as_floating   () noexcept { return value_as<bu::Float >(); }
        inline auto& as_character  () noexcept { return value_as<char      >(); }
        inline auto& as_boolean    () noexcept { return value_as<bool      >(); }
        inline auto& as_string     () noexcept { return value_as<String    >(); }
        inline auto& as_identifier () noexcept { return value_as<Identifier>(); }
    };

    static_assert(std::is_trivially_copyable_v<Token>);

}