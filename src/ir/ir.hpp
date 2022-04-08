#pragma once

#include "bu/utilities.hpp"
#include "bu/wrapper.hpp"
#include "bu/flatmap.hpp"
#include "bu/bounded_integer.hpp"
#include "ast/ast.hpp" // fix?
#include "vm/virtual_machine.hpp"


namespace resolution {

    struct Namespace; // Forward declared because Struct and Data definitions need associated namespaces

}


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
            std::string             name;
            std::vector<Parameter>  parameters;
            bu::Wrapper<Type>       return_type;
            bu::Wrapper<Expression> body;
        };

        struct Data_constructor {
            std::optional<bu::Wrapper<Type>> payload_type;
            bu::Wrapper<Type>                function_type;
            bu::Wrapper<Type>                data_type;
            lexer::Identifier                name;
            bu::U8                           tag;

            DEFAULTED_EQUALITY(Data_constructor);
        };

        struct Data {
            std::string                        name;
            bu::Wrapper<resolution::Namespace> associated_namespace;
            Size_type                          size;
            bool                               is_trivial = false;
        };

        struct Struct {
            struct Member {
                bu::Wrapper<Type> type;
                bu::U16           offset;
                bool              is_public = false;
            };
            bu::Flatmap<lexer::Identifier, Member> members;
            std::string                            name;
            bu::Wrapper<resolution::Namespace>     associated_namespace;
            Size_type                              size;
            bool                                   is_trivial = false;
        };

        struct Alias {
            std::string       name; // Unnecessary?
            bu::Wrapper<Type> type;
        };

        struct Typeclass {
            std::string name;
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
            std::vector<bu::Wrapper<Type>> parameter_types;
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

            auto operator==(User_defined_data const& other) const noexcept -> bool {
                return std::to_address(data) == std::to_address(other.data);
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

        auto hash() const -> bu::Usize;

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

        struct Data_constructor_reference {
            bu::Wrapper<definition::Data_constructor> constructor;
            DEFAULTED_EQUALITY(Data_constructor_reference);
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
            expression::Invocation,
            expression::Struct_initializer,
            expression::Let_binding,
            expression::Local_variable,
            expression::Function_reference,
            expression::Data_constructor_reference,
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

        auto hash() const -> bu::Usize;

        DEFAULTED_EQUALITY(Expression);
    };

    inline bu::Wrapper<Expression> const unit_value { expression::Tuple {}, type::unit };


    struct Template_argument_set {
        template <class T>
        using Table = bu::Flatmap<lexer::Identifier, T>;

        Table<ir::Expression>        expression_arguments;
        Table<bu::Wrapper<ir::Type>> type_arguments;
        Table<bool>                  mutability_arguments;

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


template <>
struct std::hash<ir::Expression> {
    auto operator()(ir::Expression const&) const -> bu::Usize;
};

template <>
struct std::hash<ir::Type> {
    auto operator()(ir::Type const&) const -> bu::Usize;
};

template <>
struct std::hash<ir::Template_argument_set> {
    auto operator()(ir::Template_argument_set const&) const -> bu::Usize;
};