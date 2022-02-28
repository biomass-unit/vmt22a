#pragma once

#include "bu/utilities.hpp"
#include "bu/wrapper.hpp"
#include "bu/source.hpp"
#include "bu/flatmap.hpp"
#include "lexer/token.hpp"


namespace ast {

    struct [[nodiscard]] Expression;
    struct [[nodiscard]] Pattern;
    struct [[nodiscard]] Type;

    struct Template_argument;

    struct Function_argument;

    struct Root_qualifier;

    struct Middle_qualifier {
        struct Lower {
            lexer::Identifier name;
            DEFAULTED_EQUALITY(Lower);
        };
        struct Upper {
            std::optional<std::vector<Template_argument>> template_arguments;
            lexer::Identifier                             name;
            DEFAULTED_EQUALITY(Upper);
        };
        std::variant<Lower, Upper> value;
        DEFAULTED_EQUALITY(Middle_qualifier);
    };

    struct Qualified_name {
        bu::Wrapper<Root_qualifier>   root_qualifier;
        std::vector<Middle_qualifier> qualifiers;
        lexer::Identifier             identifier;
        DEFAULTED_EQUALITY(Qualified_name);
    };

    struct Mutability {
        enum class Type { mut, immut, parameterized };

        std::optional<lexer::Identifier> parameter_name;
        Type                             type;
        DEFAULTED_EQUALITY(Mutability);
    };

}


#define DEFINE_NODE_CTOR(name)                                      \
template <class X> requires (!bu::similar_to<X, name>)              \
name(X&& x) noexcept(std::is_nothrow_constructible_v<Variant, X&&>) \
    : value { std::forward<X>(x) } {}

#include "nodes/expression.hpp"
#include "nodes/pattern.hpp"
#include "nodes/type.hpp"
#include "nodes/definition.hpp"
#include "nodes/namespace.hpp"

#undef DEFINE_NODE_CTOR


struct ast::Function_argument {
    Expression                       expression;
    std::optional<lexer::Identifier> name;
    DEFAULTED_EQUALITY(Function_argument);
};

struct ast::Root_qualifier {
    struct Global { DEFAULTED_EQUALITY(Global); };
    std::variant<
        std::monostate,    // id
        Global,            // ::id
        Type               // Type::id
    > value;
    DEFAULTED_EQUALITY(Root_qualifier);
};


namespace ast {

    inline bu::Wrapper<Expression> const unit_value =       Tuple {};
    inline bu::Wrapper<Type      > const unit_type  = type::Tuple {};

    struct [[nodiscard]] Module {
        bu::Source source;
        Namespace global_namespace;
    };

}