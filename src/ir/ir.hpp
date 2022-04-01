#pragma once

#include "bu/utilities.hpp"
#include "bu/wrapper.hpp"
#include "bu/flatmap.hpp"
#include "bu/bounded_integer.hpp"
#include "ast/ast.hpp" // fix?
#include "vm/virtual_machine.hpp"


namespace ir {

    using Size_type = bu::Bounded_integer<
        bu::Usize,
        std::numeric_limits<vm::Local_size_type>::min(),
        std::numeric_limits<vm::Local_size_type>::max()
    >;

    struct [[nodiscard]] Type;
    struct [[nodiscard]] Expression;


    namespace definition {

        struct Function {
            struct Parameter {
                bu::Wrapper<Type>                      type;
                std::optional<bu::Wrapper<Expression>> default_value;
            };
            std::vector<Parameter>  parameter_types;
            bu::Wrapper<Type>       return_type;
            bu::Wrapper<Expression> body;
        };

        struct Data {
            struct Constructor {
                std::optional<bu::Wrapper<Type>> type;
                bu::U8                           tag;
            };
            bu::Flatmap<lexer::Identifier, Constructor> constructors;
            std::string                                 name;
            Size_type                                   size;
        };

        struct Struct {
            struct Member {
                bu::Wrapper<Type> type;
                bu::U16           offset;
            };
            bu::Flatmap<lexer::Identifier, Member> members;
            std::string                            name;
            Size_type                              size;
        };

        struct Alias {

        };

        struct Typeclass {

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

        struct Pointer {
            bu::Wrapper<Type> type;
            bool              mut = false;
            DEFAULTED_EQUALITY(Pointer);
        };

        struct Reference {
            Pointer pointer;
            DEFAULTED_EQUALITY(Reference);
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

        Variant   value;
        Size_type size;
        bool      is_trivial = false;

        //auto conforms_to(Typeclass) -> bool

        auto is_unit() const noexcept -> bool;

        DEFAULTED_EQUALITY(Type);
    };

    namespace type {

        namespace dtl {
            template <class T>
            bu::Wrapper<Type> const make_primitive = Type {
                .value      = type::Primitive<T> {},
                .size       = Size_type { bu::unchecked_tag, sizeof(T) },
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
            .is_trivial = true
        };

    }


    namespace expression {

        template <class T>
        using Literal = ast::expression::Literal<T>;

        struct Array_literal {
            std::vector<Expression> elements;
            DEFAULTED_EQUALITY(Array_literal);
        };

        struct Tuple {
            std::vector<Expression> expressions;
            DEFAULTED_EQUALITY(Tuple);
        };

        struct Let_binding {
            bu::Wrapper<Expression> initializer;
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
            bu::Bounded_u16         offset;
            DEFAULTED_EQUALITY(Member_access);
        };

        struct Conditional {
            bu::Wrapper<Expression> condition;
            bu::Wrapper<Expression> true_branch;
            bu::Wrapper<Expression> false_branch;
            DEFAULTED_EQUALITY(Conditional);
        };

        struct Type_cast {
            bu::Wrapper<Expression> expression;
            bu::Wrapper<Type>       type;
            DEFAULTED_EQUALITY(Type_cast);
        };

        struct Infinite_loop {
            bu::Wrapper<Expression> body;
            DEFAULTED_EQUALITY(Infinite_loop);
        };

        struct While_loop {
            bu::Wrapper<Expression> condition;
            bu::Wrapper<Expression> body;
            DEFAULTED_EQUALITY(While_loop);
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
            expression::Array_literal,
            expression::Tuple,
            expression::Let_binding,
            expression::Local_variable,
            expression::Reference,
            expression::Member_access,
            expression::Conditional,
            expression::Type_cast,
            expression::Infinite_loop,
            expression::While_loop,
            expression::Compound
        >;

        Variant           value;
        bu::Wrapper<Type> type;

        DEFAULTED_EQUALITY(Expression);
    };

    inline bu::Wrapper<Expression> const unit_value { expression::Tuple {}, type::unit };


    struct Program {};

}