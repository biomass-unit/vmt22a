#include "bu/utilities.hpp"
#include "ast/ast.hpp"
#include "hir.hpp"


// This translation unit defines formatters for the components that are shared
// between the AST and the HIR, and explicitly instantiates them for both cases.



template <ast::tree_configuration Configuration>
DEFINE_FORMATTER_FOR(ast::Basic_name<Configuration>) {
    return std::format_to(context.out(), "{}", value.identifier.view());
}

template struct std::formatter<ast::Name>;
template struct std::formatter<hir::Name>;



template <ast::tree_configuration Configuration>
DEFINE_FORMATTER_FOR(ast::Basic_template_argument<Configuration>) {
    if (value.name) {
        std::format_to(context.out(), "{} = ", *value.name);
    }
    return std::visit(bu::Overload {
        [&](ast::Mutability const& mutability) {
            switch (mutability.type) {
            case ast::Mutability::Type::mut:
                return std::format_to(context.out(), "mut");
            case ast::Mutability::Type::immut:
                return std::format_to(context.out(), "immut");
            case ast::Mutability::Type::parameterized:
                return std::format_to(context.out(), "mut?{}", mutability.parameter_name);
            default:
                std::unreachable();
            }
        },
        [&](auto const& argument) {
            return std::format_to(context.out(), "{}", argument);
        }
    }, value.value);
}

template struct std::formatter<ast::Template_argument>;
template struct std::formatter<hir::Template_argument>;



template <ast::tree_configuration Configuration>
DEFINE_FORMATTER_FOR(ast::Basic_qualified_name<Configuration>) {
    auto out = context.out();

    std::visit(bu::Overload {
        [   ](std::monostate)                                   {},
        [out](ast::Basic_root_qualifier<Configuration>::Global) { std::format_to(out, "::"        ); },
        [out](auto const& root)                                { std::format_to(out, "{}::", root); }
        }, value.root_qualifier.value);

    for (auto& qualifier : value.middle_qualifiers) {
        std::format_to(
            out,
            "{}{}::",
            qualifier.name,
            qualifier.template_arguments ? std::format("[{}]", qualifier.template_arguments) : ""
        );
    }

    return std::format_to(out, "{}", value.primary_name.identifier);
}

template struct std::formatter<ast::Qualified_name>;
template struct std::formatter<hir::Qualified_name>;



template <ast::tree_configuration Configuration>
DEFINE_FORMATTER_FOR(ast::Basic_class_reference<Configuration>) {
    return value.template_arguments
        ? std::format_to(context.out(), "{}[{}]", value.name, value.template_arguments)
        : std::format_to(context.out(), "{}", value.name);
};

template struct std::formatter<ast::Class_reference>;
template struct std::formatter<hir::Class_reference>;



template <ast::tree_configuration Configuration>
DEFINE_FORMATTER_FOR(ast::Basic_template_parameter<Configuration>) {
    std::format_to(context.out(), "{}", value.name);

    return std::visit(
        bu::Overload {
            [&](ast::Basic_template_parameter<Configuration>::Type_parameter const& parameter) {
                if (!parameter.classes.empty()) {
                    std::format_to(
                        context.out(),
                        ": {}",
                        bu::fmt::delimited_range(parameter.classes, " + ")
                    );
                }
                return context.out();
            },
            [&](ast::Basic_template_parameter<Configuration>::Value_parameter const& parameter) {
                return std::format_to(context.out(), "{}", parameter.type.transform(": {}"_format).value_or(""));
            },
            [&](ast::Basic_template_parameter<Configuration>::Mutability_parameter const&) {
                return std::format_to(context.out(), ": mut");
            }
        },
        value.value
    );
}

template struct std::formatter<ast::Template_parameter>;
template struct std::formatter<hir::Template_parameter>;



template <ast::tree_configuration Configuration>
DEFINE_FORMATTER_FOR(ast::Basic_template_parameters<Configuration>) {
    return std::format_to(context.out(), "{}", value.vector.transform("[{}]"_format).value_or(""));
}

template struct std::formatter<ast::Template_parameters>;
template struct std::formatter<hir::Template_parameters>;



// ^^^ General components ^^^
// vvv Definition nodes   vvv


template <ast::tree_configuration Configuration>
DECLARE_FORMATTER_FOR_TEMPLATE(ast::definition::Basic_struct_member<Configuration>);
template <ast::tree_configuration Configuration>
DECLARE_FORMATTER_FOR_TEMPLATE(ast::definition::Basic_enum_constructor<Configuration>);

template <ast::tree_configuration Configuration>
DEFINE_FORMATTER_FOR(ast::definition::Basic_struct_member<Configuration>) {
    return std::format_to(
        context.out(),
        "{}{}: {}",
        value.is_public ? "pub " : "",
        value.name,
        value.type
    );
}

template <ast::tree_configuration Configuration>
DEFINE_FORMATTER_FOR(ast::definition::Basic_enum_constructor<Configuration>) {
    return std::format_to(context.out(), "{}({})", value.name, value.type);
}


template <ast::tree_configuration Configuration>
DEFINE_FORMATTER_FOR(ast::definition::Basic_struct<Configuration>) {
    return std::format_to(
        context.out(),
        "struct {}{} = {}",
        value.name,
        value.template_parameters,
        value.members
    );
}

template struct std::formatter<ast::definition::Struct>;
template struct std::formatter<hir::definition::Struct>;


template <ast::tree_configuration Configuration>
DEFINE_FORMATTER_FOR(ast::definition::Basic_enum<Configuration>) {
    return std::format_to(
        context.out(),
        "enum {}{} = {}",
        value.name,
        value.template_parameters,
        value.constructors
    );
}

template struct std::formatter<ast::definition::Enum>;
template struct std::formatter<hir::definition::Enum>;


template <ast::tree_configuration Configuration>
DEFINE_FORMATTER_FOR(ast::definition::Basic_alias<Configuration>) {
    return std::format_to(
        context.out(),
        "alias {}{} = {}",
        value.name,
        value.template_parameters,
        value.type
    );
}

template struct std::formatter<ast::definition::Alias>;
template struct std::formatter<hir::definition::Alias>;


template <ast::tree_configuration Configuration>
DEFINE_FORMATTER_FOR(ast::definition::Basic_typeclass<Configuration>) {
    std::format_to(
        context.out(),
        "class {}{} {{",
        value.name,
        value.template_parameters
    );

    for (auto& signature : value.function_signatures) {
        std::format_to(
            context.out(),
            "fn {}{}({}): {}\n",
            signature.name,
            signature.template_parameters,
            signature.argument_types,
            signature.return_type
        );
    }
    for (auto& signature : value.type_signatures) {
        std::format_to(
            context.out(),
            "alias {}{}{}\n",
            signature.name,
            signature.template_parameters,
            signature.classes.empty() ? "" : ": {}"_format(bu::fmt::delimited_range(signature.classes, " + "))
        );
    }

    return std::format_to(context.out(), "}}");
}

template struct std::formatter<ast::definition::Typeclass>;
template struct std::formatter<hir::definition::Typeclass>;


template <ast::tree_configuration Configuration>
DEFINE_FORMATTER_FOR(ast::definition::Basic_implementation<Configuration>) {
    return std::format_to(
        context.out(),
        "impl {} {{\n{}\n}}",
        value.type,
        bu::fmt::delimited_range(value.definitions, "\n\n")
    );
}

template struct std::formatter<ast::definition::Implementation>;
template struct std::formatter<hir::definition::Implementation>;


template <ast::tree_configuration Configuration>
DEFINE_FORMATTER_FOR(ast::definition::Basic_instantiation<Configuration>) {
    return std::format_to(
        context.out(),
        "inst {} {} {{\n{}\n}}",
        value.typeclass,
        value.instance,
        bu::fmt::delimited_range(value.definitions, "\n\n")
    );
}

template struct std::formatter<ast::definition::Instantiation>;
template struct std::formatter<hir::definition::Instantiation>;


template <ast::tree_configuration Configuration>
DEFINE_FORMATTER_FOR(ast::definition::Basic_namespace<Configuration>) {
    return std::format_to(
        context.out(),
        "namespace {}{} {{\n{}\n}}",
        value.name,
        value.template_parameters,
        bu::fmt::delimited_range(value.definitions, "\n\n")
    );
}

template struct std::formatter<ast::definition::Namespace>;
template struct std::formatter<hir::definition::Namespace>;