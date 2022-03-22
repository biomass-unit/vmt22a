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

    struct Qualifier {
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
        DEFAULTED_EQUALITY(Qualifier);

        auto lower() noexcept -> Lower* { return std::get_if<Lower>(&value); }
        auto upper() noexcept -> Upper* { return std::get_if<Upper>(&value); }
    };

    struct Primary_qualifier {
        lexer::Identifier identifier;
        bool              uppercase;
        DEFAULTED_EQUALITY(Primary_qualifier);

        auto is_upper() const noexcept -> bool { return uppercase; }
        auto is_lower() const noexcept -> bool { return !uppercase; }
    };

    struct Qualified_name {
        bu::Wrapper<Root_qualifier> root_qualifier;
        std::vector<Qualifier>      middle_qualifiers;
        Primary_qualifier           primary_qualifier;
        DEFAULTED_EQUALITY(Qualified_name);

        inline auto is_unqualified() const noexcept -> bool;
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
        std::monostate, // id, id::id
        Global,         // ::id
        Type            // Type::id
    > value;
    DEFAULTED_EQUALITY(Root_qualifier);
};

auto ast::Qualified_name::is_unqualified() const noexcept -> bool {
    return middle_qualifiers.empty()
        && std::holds_alternative<std::monostate>(root_qualifier->value);
}


namespace ast {

    inline bu::Wrapper<Expression> const unit_value = expression::Tuple {};

    using Module_path = std::vector<lexer::Identifier>;

    struct Import {
        Module_path                      path;
        std::optional<lexer::Identifier> alias;
    };

    struct [[nodiscard]] Module {
        bu::Source              source;
        std::vector<Definition> definitions;

        std::optional<lexer::Identifier> name;
        std::vector<Import>              imports;
        std::vector<Module_path>         imported_by;
    };

}