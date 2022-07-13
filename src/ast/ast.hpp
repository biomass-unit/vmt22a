#pragma once

#include "bu/utilities.hpp"
#include "bu/wrapper.hpp"
#include "bu/source.hpp"
#include "bu/diagnostics.hpp"
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

    struct Function_parameter;

    struct Template_argument;


    struct Qualifier {
        std::optional<std::vector<Template_argument>> template_arguments;
        Name                                          name;

        bu::Source_view source_view;
        DEFAULTED_EQUALITY(Qualifier);
    };

    template <class Type>
    struct Basic_root_qualifier {
        struct Global { DEFAULTED_EQUALITY(Global); };
        std::variant<
            std::monostate,   // id, id::id
            Global,           // ::id
            bu::Wrapper<Type> // Type::id
        > value;
        DEFAULTED_EQUALITY(Basic_root_qualifier);
    };

    template <class Type>
    struct Basic_qualified_name {
        std::vector<Qualifier>      middle_qualifiers;
        Basic_root_qualifier<Type>  root_qualifier;
        Name                        primary_name;

        DEFAULTED_EQUALITY(Basic_qualified_name);

        inline auto is_unqualified() const noexcept -> bool;
    };

    using Root_qualifier = Basic_root_qualifier<Type>;
    using Qualified_name = Basic_qualified_name<Type>;


    struct Mutability {
        enum class Type { mut, immut, parameterized };

        std::optional<lexer::Identifier> parameter_name;
        Type                             type;

        bu::Source_view source_view;
        DEFAULTED_EQUALITY(Mutability);
    };


    template <class Type>
    struct Basic_class_reference {
        std::optional<std::vector<Template_argument>> template_arguments;
        Basic_qualified_name<Type>                    name;

        bu::Source_view source_view;
        DEFAULTED_EQUALITY(Basic_class_reference);
    };

    using Class_reference = Basic_class_reference<Type>;


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
    Expression          expression;
    std::optional<Name> name;
    DEFAULTED_EQUALITY(Function_argument);
};

struct ast::Function_parameter {
    bu::Wrapper<Pattern>      pattern;
    std::optional<Type>       type;
    std::optional<Expression> default_value;
    DEFAULTED_EQUALITY(Function_parameter);
};

struct ast::Template_argument {
    std::variant<Type, Expression, Mutability> value;
    std::optional<Name>                        name;
    DEFAULTED_EQUALITY(Template_argument);
};

template <class Type>
auto ast::Basic_qualified_name<Type>::is_unqualified() const noexcept -> bool {
    return middle_qualifiers.empty()
        && std::holds_alternative<std::monostate>(root_qualifier.value);
}


namespace ast {

    using Node_context = bu::Wrapper_context_for<
        ast::Expression,
        ast::Type,
        ast::Pattern,
        ast::Definition
    >;


    struct Module_path {
        std::vector<lexer::Identifier> components;
        lexer::Identifier              module_name;
    };

    struct [[nodiscard]] Import {
        Module_path                      path;
        std::optional<lexer::Identifier> alias;
    };

    struct [[nodiscard]] Module {
        Node_context             node_context;
        bu::diagnostics::Builder diagnostics;
        bu::Source               source;
        std::vector<Definition>  definitions;

        std::optional<lexer::Identifier> name;
        std::vector<Import>              imports;
        std::vector<Module_path>         imported_by;
    };


    struct [[nodiscard]] Full_program {
        Node_context                    node_context;
        bu::Wrapper_context<bu::Source> source_context;

        std::vector<bu::Pair<bu::Wrapper<bu::Source>, Definition>> definitions;
    };

}


DECLARE_FORMATTER_FOR(ast::Expression);
DECLARE_FORMATTER_FOR(ast::Pattern);
DECLARE_FORMATTER_FOR(ast::Type);
DECLARE_FORMATTER_FOR(ast::Definition);

DECLARE_FORMATTER_FOR(ast::Module);
DECLARE_FORMATTER_FOR(ast::Name);
DECLARE_FORMATTER_FOR(ast::Mutability);
DECLARE_FORMATTER_FOR(ast::Template_argument);


// These are explicitly instantiated in hir/hir_formatting.cpp
template <class Type>
DECLARE_FORMATTER_FOR_TEMPLATE(ast::Basic_qualified_name<Type>);
template <class Type>
DECLARE_FORMATTER_FOR_TEMPLATE(ast::Basic_class_reference<Type>);