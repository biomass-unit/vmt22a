#pragma once

#include "bu/utilities.hpp"
#include "bu/wrapper.hpp"
#include "bu/source.hpp"
#include "bu/diagnostics.hpp"
#include "bu/flatmap.hpp"
#include "lexer/token.hpp"


namespace ast {

    struct Name {
        lexer::Identifier identifier;
        bool              is_upper;
        bu::Source_view   source_view;

        auto operator==(Name const& other) const noexcept -> bool {
            return identifier == other.identifier;
        }
    };

    struct Mutability {
        enum class Type { mut, immut, parameterized };

        std::optional<lexer::Identifier> parameter_name;
        Type                             type;

        bu::Source_view source_view;
        DEFAULTED_EQUALITY(Mutability);
    };


    struct [[nodiscard]] Expression;
    struct [[nodiscard]] Pattern;
    struct [[nodiscard]] Type;
    struct [[nodiscard]] Definition;


    template <class T>
    concept tree_configuration = requires {
        typename T::Expression;
        typename T::Pattern;
        typename T::Type;
        typename T::Definition;
    };


    struct Function_argument;

    struct Function_parameter;


    template <tree_configuration Configuration>
    struct Basic_template_argument {
        using Variant = std::variant<
            bu::Wrapper<typename Configuration::Type>,
            bu::Wrapper<typename Configuration::Expression>,
            Mutability
        >;
        Variant             value;
        std::optional<Name> name;
        DEFAULTED_EQUALITY(Basic_template_argument);
    };

    template <tree_configuration Configuration>
    struct Basic_qualifier {
        std::optional<std::vector<Basic_template_argument<Configuration>>> template_arguments;
        Name                                                               name;

        bu::Source_view source_view;
        DEFAULTED_EQUALITY(Basic_qualifier);
    };

    template <tree_configuration Configuration>
    struct Basic_root_qualifier {
        struct Global { DEFAULTED_EQUALITY(Global); };
        std::variant<
            std::monostate,                           // id, id::id
            Global,                                   // ::id
            bu::Wrapper<typename Configuration::Type> // Type::id
        > value;
        DEFAULTED_EQUALITY(Basic_root_qualifier);
    };

    template <tree_configuration Configuration>
    struct Basic_qualified_name {
        std::vector<Basic_qualifier<Configuration>> middle_qualifiers;
        Basic_root_qualifier<Configuration>         root_qualifier;
        Name                                        primary_name;

        DEFAULTED_EQUALITY(Basic_qualified_name);

        inline auto is_unqualified() const noexcept -> bool;
    };

    template <tree_configuration Configuration>
    struct Basic_class_reference {
        std::optional<std::vector<Basic_template_argument<Configuration>>> template_arguments;
        Basic_qualified_name<Configuration>                                name;

        bu::Source_view source_view;
        DEFAULTED_EQUALITY(Basic_class_reference);
    };


    struct AST_configuration {
        using Expression = ::ast::Expression;
        using Pattern    = ::ast::Pattern;
        using Type       = ::ast::Type;
        using Definition = ::ast::Definition;
    };

    using Template_argument = Basic_template_argument<AST_configuration>;
    using Qualifier         = Basic_qualifier        <AST_configuration>;
    using Root_qualifier    = Basic_root_qualifier   <AST_configuration>;
    using Qualified_name    = Basic_qualified_name   <AST_configuration>;
    using Class_reference   = Basic_class_reference  <AST_configuration>;


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
    Pattern                   pattern;
    std::optional<Type>       type;
    std::optional<Expression> default_value;
    DEFAULTED_EQUALITY(Function_parameter);
};

template <ast::tree_configuration Configuration>
auto ast::Basic_qualified_name<Configuration>::is_unqualified() const noexcept -> bool {
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


// These are explicitly instantiated in hir/hir_formatting.cpp to avoid repetition
template <ast::tree_configuration Configuration>
DECLARE_FORMATTER_FOR_TEMPLATE(ast::Basic_template_argument<Configuration>);
template <ast::tree_configuration Configuration>
DECLARE_FORMATTER_FOR_TEMPLATE(ast::Basic_qualified_name<Configuration>);
template <ast::tree_configuration Configuration>
DECLARE_FORMATTER_FOR_TEMPLATE(ast::Basic_class_reference<Configuration>);