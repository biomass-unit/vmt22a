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
    struct [[nodiscard]] Definition;


    struct Name {
        lexer::Identifier identifier;
        bool              is_upper;
        bu::Source_view   source_view;

        auto operator==(Name const& other) const noexcept -> bool {
            return identifier == other.identifier;
        }
    };


    struct Function_argument;

    struct Template_argument;


    struct Qualifier {
        std::optional<std::vector<Template_argument>> template_arguments;
        Name                                          name;

        bu::Source_view source_view;
        DEFAULTED_EQUALITY(Qualifier);
    };

    struct Root_qualifier;

    struct Qualified_name {
        std::vector<Qualifier>      middle_qualifiers;
        bu::Wrapper<Root_qualifier> root_qualifier;
        Name                        primary_name;

        DEFAULTED_EQUALITY(Qualified_name);

        inline auto is_unqualified() const noexcept -> bool;
    };


    struct Mutability {
        enum class Type { mut, immut, parameterized };

        std::optional<lexer::Identifier> parameter_name;
        Type                             type;

        bu::Source_view source_view;
        DEFAULTED_EQUALITY(Mutability);
    };


    template <bu::one_of<Expression, Pattern, Type, Definition> T>
    auto operator==(T const& lhs, T const& rhs) noexcept -> bool {
        return lhs.value == rhs.value;
    }

}


#include "nodes/expression.hpp"
#include "nodes/pattern.hpp"
#include "nodes/type.hpp"
#include "nodes/definition.hpp"


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