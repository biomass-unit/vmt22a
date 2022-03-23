#pragma once

#include "bu/utilities.hpp"
#include "bu/wrapper.hpp"
#include "lexer/token.hpp" // for lexer::Identifier, fix?
#include "ast/ast.hpp" // fix?
#include "vm/virtual_machine.hpp"


namespace ir {

    struct [[nodiscard]] Type;
    struct [[nodiscard]] Expression;


    namespace definition {

        struct Data {
            // should the constructors be stored here, or in a flatmap outside the struct?
            std::string name;
            bu::Usize   size;
        };

        struct Struct {
            struct Member {
                lexer::Identifier name;
                bu::Wrapper<Type> type;
                DEFAULTED_EQUALITY(Member);
            };
            std::string         name;
            std::vector<Member> members;
            bu::Usize           size;
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
            std::vector<Type> types;
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
            DEFAULTED_EQUALITY(User_defined_data);
        };

        struct User_defined_struct {
            bu::Wrapper<definition::Struct> structure;
            DEFAULTED_EQUALITY(User_defined_struct);
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

        //auto conforms_to(Typeclass) -> bool

        DEFAULTED_EQUALITY(Type);
    };


    namespace expression {

        template <class T>
        using Literal = ast::expression::Literal<T>;

        struct Local_variable {
            bu::Wrapper<Type>     type;
            vm::Local_offset_type frame_offset;
            DEFAULTED_EQUALITY(Local_variable);
        };

    }

    struct Expression {
        using Variant = std::variant<
            expression::Literal<bu::Isize>,
            expression::Literal<bu::Float>,
            expression::Literal<bu::Char>,
            expression::Literal<bool>,
            expression::Literal<lexer::String>,
            expression::Local_variable
        >;

        Variant           value;
        bu::Wrapper<Type> type;

        DEFAULTED_EQUALITY(Expression);
    };


    struct Program {};

}