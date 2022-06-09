#pragma once

#include "bu/utilities.hpp"
#include "bu/wrapper.hpp"
#include "bu/flatmap.hpp"
#include "lexer/token.hpp"


namespace tst {

    struct [[nodiscard]] Type;
    struct [[nodiscard]] Expression;


    namespace definition {

        struct Namespace;


        struct Function {
            struct Parameter {
                bu::Wrapper<Type>                      type;
                std::optional<bu::Wrapper<Expression>> default_value;
            };

            std::string             name;
            std::vector<Parameter>  parameters;
            bu::Wrapper<Type>       return_type;
            bu::Wrapper<Type>       function_type;
            bu::Wrapper<Expression> body;
            bu::Wrapper<Namespace>  home_namespace;
        };


        struct Enum {
            struct Constructor {
                std::optional<bu::Wrapper<Type>> payload_type;
                bu::Wrapper<Type>                function_type;
                lexer::Identifier                name;
                bu::U8                           tag;

                bu::Source_view source_view;
            };

            std::string                           name;
            std::vector<bu::Wrapper<Constructor>> constructors;
            bu::Wrapper<Type>                     enum_type;
            bu::Wrapper<Namespace>                home_namespace;
        };


        struct Struct {
            struct Member {
                bu::Wrapper<Type> type;
                bool              is_public = false;

                bu::Source_view source_view;
            };

            std::string                            name;
            bu::Flatmap<lexer::Identifier, Member> members;
            bu::Wrapper<Type>                      struct_type;
            bu::Wrapper<Namespace>                 home_namespace;
        };


        struct Alias {
            bu::Wrapper<Type> type;
        };


        struct Typeclass {
            std::string            name;
            bu::Wrapper<Namespace> home_namespace;
        };

    }


    namespace type {

        template <class>
        struct Primitive {
            DEFAULTED_EQUALITY(Primitive);
        };

        using Integer   = Primitive<bu::Isize>;
        using Floating  = Primitive<bu::Float>;
        using Character = Primitive<bu::Char>;
        using Boolean   = Primitive<bool>;
        using String    = Primitive<lexer::String>;

        struct Tuple {
            std::vector<bu::Wrapper<Type>> types;
            DEFAULTED_EQUALITY(Tuple);
        };

        struct Function {
            std::vector<bu::Wrapper<Type>> parameter_types;
            bu::Wrapper<Type>              return_type;
            DEFAULTED_EQUALITY(Function);
        };

        struct Array {
            bu::Wrapper<Type> element_type;
            bu::Usize         length;
            DEFAULTED_EQUALITY(Array);
        };

        struct Slice {
            bu::Wrapper<Type> element_type;
            DEFAULTED_EQUALITY(Slice);
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

        struct User_defined_enum {
            bu::Wrapper<definition::Enum> enumeration;

            auto operator==(User_defined_enum const& other) const noexcept -> bool {
                return std::to_address(enumeration) == std::to_address(other.enumeration);
            }
        };

        struct User_defined_struct {
            bu::Wrapper<definition::Struct> structure;

            auto operator==(User_defined_struct const& other) const noexcept -> bool {
                return std::to_address(structure) == std::to_address(other.structure);
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
            type::Slice,
            type::Reference,
            type::Pointer,
            type::User_defined_enum,
            type::User_defined_struct
        >;

        Variant value;

        auto hash() const -> bu::Usize;

        auto is_unit() const noexcept -> bool;

        DEFAULTED_EQUALITY(Type);
    };

    namespace type {

        namespace dtl {
            template <class T>
            bu::Wrapper<Type> const make_primitive = Type { type::Primitive<T> {} };
        }

        inline auto const
            integer   = dtl::make_primitive<bu::Isize    >,
            floating  = dtl::make_primitive<bu::Float    >,
            character = dtl::make_primitive<bu::Char     >,
            boolean   = dtl::make_primitive<bool         >,
            string    = dtl::make_primitive<lexer::String>;

        inline bu::Wrapper<Type> const unit = Type { type::Tuple {} };

    }


    namespace expression {

        template <class T>
        struct Literal {
            T value;
            DEFAULTED_EQUALITY(Literal);
        };

        using Integer   = Literal<bu::Isize>;
        using Floating  = Literal<bu::Float>;
        using Character = Literal<bu::Char>;
        using Boolean   = Literal<bool>;
        using String    = Literal<lexer::String>;

        struct Array_literal {
            std::vector<Expression> elements;
            DEFAULTED_EQUALITY(Array_literal);
        };

        struct Tuple {
            std::vector<Expression> expressions;
            DEFAULTED_EQUALITY(Tuple);
        };

        struct Invocation {
            std::vector<Expression> arguments;
            bu::Wrapper<Expression> invocable;
            DEFAULTED_EQUALITY(Invocation);
        };

        struct Struct_initializer {
            std::vector<Expression> member_initializers;
            bu::Wrapper<Type>       type;
            DEFAULTED_EQUALITY(Struct_initializer);
        };

        struct Let_binding {
            bu::Wrapper<Expression> initializer;
            DEFAULTED_EQUALITY(Let_binding);
        };

        struct Local_variable {
            bu::U16 frame_offset;
            DEFAULTED_EQUALITY(Local_variable);
        };

        struct Function_reference {
            bu::Wrapper<definition::Function> definition;
            DEFAULTED_EQUALITY(Function_reference);
        };

        struct Enum_constructor_reference {
            bu::Wrapper<definition::Enum::Constructor> constructor;
            bu::Wrapper<definition::Enum>              enumeration;
            DEFAULTED_EQUALITY(Enum_constructor_reference);
        };

        struct Reference {
            bu::U16 frame_offset;
            DEFAULTED_EQUALITY(Reference);
        };

        struct Struct_member_access {
            bu::Wrapper<Expression> expression;
            lexer::Identifier       member_name;
            DEFAULTED_EQUALITY(Struct_member_access);
        };

        struct Tuple_member_access {
            bu::Wrapper<Expression> expression;
            bu::Usize               member_index;
            DEFAULTED_EQUALITY(Tuple_member_access);
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
            expression::Invocation,
            expression::Struct_initializer,
            expression::Let_binding,
            expression::Local_variable,
            expression::Function_reference,
            expression::Enum_constructor_reference,
            expression::Reference,
            expression::Struct_member_access,
            expression::Tuple_member_access,
            expression::Conditional,
            expression::Type_cast,
            expression::Infinite_loop,
            expression::While_loop,
            expression::Compound
        >;

        Variant           value;
        bu::Wrapper<Type> type;

        auto hash() const -> bu::Usize;

        DEFAULTED_EQUALITY(Expression);
    };

    inline bu::Wrapper<Expression> const unit_value { expression::Tuple {}, type::unit };


    struct Template_argument_set {
        template <class T>
        using Table = bu::Flatmap<lexer::Identifier, T>;

        Table<Expression>        expression_arguments;
        Table<bu::Wrapper<Type>> type_arguments;
        Table<bool>              mutability_arguments;

        struct Argument_indicator {
            enum class Kind { expression, type, mutability } kind;
            bu::Usize                                        index;
        };
        std::vector<Argument_indicator> arguments_in_order;

        auto append_formatted_arguments_to(std::string&) const -> void;

        auto hash() const -> bu::Usize;
    };


    struct Program {};

}