#pragma once

#include "bu/utilities.hpp"
#include "ast/ast.hpp"


namespace hir {

    struct [[nodiscard]] Expression;
    struct [[nodiscard]] Type;
    struct [[nodiscard]] Pattern;
    struct [[nodiscard]] Definition;

    struct [[nodiscard]] Function_argument;
    struct [[nodiscard]] Function_parameter;
    struct [[nodiscard]] Template_parameter;

    struct [[nodiscard]] Template_parameters {
        std::optional<std::vector<Template_parameter>> vector;
        Template_parameters(decltype(vector)&& vector) noexcept
            : vector { vector } {}
    };

}


#include "nodes/expression.hpp"
#include "nodes/pattern.hpp"
#include "nodes/type.hpp"
#include "nodes/definition.hpp"


struct hir::Function_argument {
    Expression               expression;
    std::optional<ast::Name> name;
    DEFAULTED_EQUALITY(Function_argument);
};

struct hir::Function_parameter {
    Pattern                   pattern;
    Type                      type;
    std::optional<Expression> default_value;
};

struct hir::Template_parameter {
    struct Type_parameter {
        std::vector<ast::Class_reference> classes;
        DEFAULTED_EQUALITY(Type_parameter);
    };
    struct Value_parameter {
        std::optional<Type> type;
        DEFAULTED_EQUALITY(Value_parameter);
    };
    struct Mutability_parameter {
        DEFAULTED_EQUALITY(Mutability_parameter);
    };

    using Variant = std::variant<
        Type_parameter,
        Value_parameter,
        Mutability_parameter
    >;

    Variant                        value;
    lexer::Identifier              name;
    std::optional<bu::Source_view> source_view;
    DEFAULTED_EQUALITY(Template_parameter);
};


namespace hir {

    struct Node_context {
        bu::Wrapper_context_for<
            Expression,
            Type,
            Pattern,
            Definition
        > wrapper_context;

        bu::Wrapper<Expression> unit_value = expression::Tuple {};
        bu::Wrapper<Type>       unit_type  = type      ::Tuple {};
    };


    struct Module {
        Node_context            node_context;
        bu::Source              source;
        std::vector<Definition> definitions;
    };

}


DECLARE_FORMATTER_FOR(hir::Expression);
DECLARE_FORMATTER_FOR(hir::Type);
DECLARE_FORMATTER_FOR(hir::Pattern);
DECLARE_FORMATTER_FOR(hir::Definition);