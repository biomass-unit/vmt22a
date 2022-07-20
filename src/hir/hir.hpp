#pragma once

#include "bu/utilities.hpp"
#include "ast/ast.hpp"


namespace hir {

    struct [[nodiscard]] Expression;
    struct [[nodiscard]] Type;
    struct [[nodiscard]] Pattern;
    struct [[nodiscard]] Definition;


    struct HIR_configuration {
        using Expression  = ::hir::Expression;
        using Pattern     = ::hir::Pattern;
        using Type        = ::hir::Type;
        using Definition  = ::hir::Definition;
        using Source_view = std::optional<bu::Source_view>;
    };

    using Name                = ast::Basic_name                <HIR_configuration>;
    using Template_argument   = ast::Basic_template_argument   <HIR_configuration>;
    using Root_qualifier      = ast::Basic_root_qualifier      <HIR_configuration>;
    using Qualifier           = ast::Basic_qualifier           <HIR_configuration>;
    using Qualified_name      = ast::Basic_qualified_name      <HIR_configuration>;
    using Class_reference     = ast::Basic_class_reference     <HIR_configuration>;
    using Template_parameter  = ast::Basic_template_parameter  <HIR_configuration>;
    using Template_parameters = ast::Basic_template_parameters <HIR_configuration>;


    struct [[nodiscard]] Function_argument;
    struct [[nodiscard]] Function_parameter;

    struct Implicit_template_parameter {
        struct Tag { bu::Usize value; };

        std::vector<Class_reference> classes;
        Tag                          tag;
    };

}


namespace mir {
    struct [[nodiscard]] Type; // For hir::Expression::type
}


#include "nodes/expression.hpp"
#include "nodes/pattern.hpp"
#include "nodes/type.hpp"
#include "nodes/definition.hpp"


struct hir::Function_argument {
    Expression          expression;
    std::optional<Name> name;
    DEFAULTED_EQUALITY(Function_argument);
};

struct hir::Function_parameter {
    Pattern                   pattern;
    Type                      type;
    std::optional<Expression> default_value;
};


#include "mir/mir.hpp" // Has to be included here to make hir::Module::type_context work


namespace hir {

    using Node_context = bu::Wrapper_context_for<Expression, Type, Pattern>;

    struct Module {
        Node_context                   node_context;
        bu::Wrapper_context<mir::Type> type_context;
        bu::diagnostics::Builder       diagnostics;
        bu::Source                     source;
        std::vector<Definition>        definitions;
    };

}


DECLARE_FORMATTER_FOR(hir::Expression);
DECLARE_FORMATTER_FOR(hir::Type);
DECLARE_FORMATTER_FOR(hir::Pattern);
DECLARE_FORMATTER_FOR(hir::Definition);