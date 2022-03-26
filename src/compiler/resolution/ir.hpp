#pragma once

#include "bu/utilities.hpp"
#include "bu/wrapper.hpp"
#include "bu/flatmap.hpp"
#include "ast/ast.hpp" // fix?
#include "vm/virtual_machine.hpp"


namespace ir {

    struct [[nodiscard]] Type;
    struct [[nodiscard]] Expression;


    namespace definition {

        struct Data {
            // should the constructors be stored here, or in a flatmap outside the struct?
            std::string name;
            bu::U16     size;
        };

        struct Struct {
            struct Member {
                lexer::Identifier name;
                bu::Wrapper<Type> type;
                bu::U16           offset;
                DEFAULTED_EQUALITY(Member);
            };
            std::string                            name;
            bu::Flatmap<lexer::Identifier, Member> members;
            bu::U16                                size;
            DEFAULTED_EQUALITY(Struct);
        };

    }


    namespace type {

        template <class T>
        using Primitive = ast::type::Primitive<T>;

        using Integer   = Primitive<bu::Isize    >;
        using Floating  = Primitive<bu::Float    >;
        using Character = Primitive<bu::Char     >;
        using Boolean   = Primitive<bool         >;
        using String    = Primitive<lexer::String>;

        struct Tuple {
            std::vector<bu::Wrapper<Type>> types;
            DEFAULTED_EQUALITY(Tuple);
        };

        struct Function {
            std::vector<Type> parameter_types;
            bu::Wrapper<Type> return_type;
            DEFAULTED_EQUALITY(Function);
        };

        struct Array {
            bu::Wrapper<Type> element_type;
            bu::Usize         length;
            DEFAULTED_EQUALITY(Array);
        };

        struct List {
            bu::Wrapper<Type> element_type;
            DEFAULTED_EQUALITY(List);
        };

        struct Reference {
            bu::Wrapper<Type> type;
            bool mut;
            DEFAULTED_EQUALITY(Reference);
        };

        struct Pointer {
            bu::Wrapper<Type> type;
            bool mut;
            DEFAULTED_EQUALITY(Pointer);
        };

        struct User_defined_data {
            bu::Wrapper<definition::Data> data;

            auto operator==(User_defined_data const&) const noexcept -> bool {
                bu::unimplemented();
            }
        };

        struct User_defined_struct {
            bu::Wrapper<definition::Struct> structure;

            auto operator==(User_defined_struct const&) const noexcept -> bool {
                bu::unimplemented();
            }
        };

    }

    struct Type {
        using Variant = std::variant<
            type::Integer,
            type::Floating,
            type::Character,
            type::Boolean,
            type::String,
            type::Tuple,
            type::Function,
            type::Array,
            type::List,
            type::Reference,
            type::Pointer,
            type::User_defined_data,
            type::User_defined_struct
        >;

        Variant value;
        bu::U16 size;
        bool    is_trivial = false;

        //auto conforms_to(Typeclass) -> bool

        DEFAULTED_EQUALITY(Type);
    };

    namespace type {

        namespace dtl {
            template <class T>
            bu::Wrapper<Type> const make_primitive = Type {
                .value      = type::Primitive<T> {},
                .size       = sizeof(T),
                .is_trivial = true
            };
        }

        inline auto const
            integer   = dtl::make_primitive<bu::Isize    >,
            floating  = dtl::make_primitive<bu::Float    >,
            character = dtl::make_primitive<bu::Char     >,
            boolean   = dtl::make_primitive<bool         >,
            string    = dtl::make_primitive<lexer::String>;

        inline bu::Wrapper<Type> const unit = Type {
            .value      = type::Tuple {},
            .size       = 0,
            .is_trivial = true
        };

    }


    namespace expression {

        template <class T>
        using Literal = ast::expression::Literal<T>;

        struct Tuple {
            std::vector<Expression> expressions;
            DEFAULTED_EQUALITY(Tuple);
        };

        struct Let_binding {
            bu::Wrapper<ast::Pattern> pattern;
            bu::Wrapper<Type>         type;
            bu::Wrapper<Expression>   initializer;
            DEFAULTED_EQUALITY(Let_binding);
        };

        struct Local_variable {
            bu::U16 frame_offset;
            DEFAULTED_EQUALITY(Local_variable);
        };

        struct Reference {
            bu::U16 frame_offset;
            DEFAULTED_EQUALITY(Reference);
        };

        struct Member_access {
            bu::Wrapper<Expression> expression;
            bu::Usize               offset;
            DEFAULTED_EQUALITY(Member_access);
        };

        struct Compound {
            std::vector<Expression> side_effects;
            bu::Wrapper<Expression> result;
            DEFAULTED_EQUALITY(Compound);
        };

    }

    struct Expression {
        using Variant = std::variant<
            expression::Literal<bu::Isize>,
            expression::Literal<bu::Float>,
            expression::Literal<bu::Char>,
            expression::Literal<bool>,
            expression::Literal<lexer::String>,
            expression::Tuple,
            expression::Let_binding,
            expression::Local_variable,
            expression::Reference,
            expression::Member_access,
            expression::Compound
        >;

        Variant           value;
        bu::Wrapper<Type> type;

        DEFAULTED_EQUALITY(Expression);
    };


    struct Program {};

}