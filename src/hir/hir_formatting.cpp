#include "bu/utilities.hpp"
#include "hir.hpp"


DIRECTLY_DEFINE_FORMATTER_FOR(hir::Function_parameter) {
    return std::format_to(
        context.out(),
        "{}: {}{}",
        value.pattern,
        value.type,
        value.default_value.transform(" = {}"_format).value_or("")
    );
}

DIRECTLY_DEFINE_FORMATTER_FOR(hir::Function_argument) {
    return value.name
        ? std::format_to(context.out(), "{} = {}", *value.name, value.expression)
        : std::format_to(context.out(), "{}", value.expression);
}

DIRECTLY_DEFINE_FORMATTER_FOR(hir::Template_parameter) {
    return std::visit(bu::Overload {
        [&](hir::Template_parameter::Value_parameter const& parameter) {
            return std::format_to(
                context.out(),
                "{}{}",
                value.name,
                parameter.type.transform(": {}"_format).value_or("")
            );
        },
        [&](hir::Template_parameter::Type_parameter const& parameter) {
            return std::vformat_to(
                context.out(),
                parameter.classes.empty() ? "{}" : "{}: {}",
                std::make_format_args(
                    value.name,
                    bu::fmt::delimited_range(parameter.classes, " + ")
                )
            );
        },
        [&](hir::Template_parameter::Mutability_parameter const&) {
            return std::format_to(context.out(), "{}: mut", value.name);
        }
    }, value.value);
}

DIRECTLY_DEFINE_FORMATTER_FOR(hir::Template_parameters) {
    return value.vector
        ? std::format_to(context.out(), "[{}]", *value.vector)
        : context.out();
}


namespace {

    struct Expression_format_visitor : bu::fmt::Visitor_base {
        auto operator()(bu::instance_of<hir::expression::Literal> auto const& literal) {
            return format("{}", literal.value);
        }
        auto operator()(hir::expression::Literal<char> const& literal) {
            return format("'{}'", literal.value);
        }
        auto operator()(hir::expression::Literal<lexer::String> const& literal) {
            return format("\"{}\"", literal.value);
        }
        auto operator()(hir::expression::Array_literal const& literal) {
            return format("[{}]", literal.elements);
        }
        auto operator()(hir::expression::Variable const& variable) {
            return format("{}", variable.name);
        }
        auto operator()(hir::expression::Tuple const& tuple) {
            return format("({})", tuple.elements);
        }
        auto operator()(hir::expression::Conditional const& conditional) {
            return format("if {} {} else {}", conditional.condition, conditional.true_branch, conditional.false_branch);
        }
        auto operator()(hir::expression::Loop const& loop) {
            return format("loop {{ {} }}", loop.body);
        }
        auto operator()(hir::expression::Break const& break_) {
            return format("break{}", break_.expression.transform(" {}"_format).value_or(""));
        }
        auto operator()(hir::expression::Continue const&) {
            return format("continue");
        }
        auto operator()(hir::expression::Block const& block) {
            format("{{ ");
            for (auto const& side_effect : block.side_effects) {
                format("{}; ", side_effect);
            }
            return format("{}}}", block.result.transform("{} "_format).value_or(""));
        }
        auto operator()(hir::expression::Invocation const& invocation) {
            return format("{}({})", invocation.invocable, invocation.arguments);
        }
        auto operator()(hir::expression::Struct_initializer const& initializer) {
            return format(
                "{} {{ {} }}",
                initializer.type,
                initializer.member_initializers.container()
            );
        }
        auto operator()(hir::expression::Binary_operator_invocation const& invocation) {
            return format("({} {} {})", invocation.left, invocation.op, invocation.right);
        }
        auto operator()(hir::expression::Match const& match) {
            format("match {} {{ ", match.expression);
            for (auto& match_case : match.cases) {
                format("{} -> {}", match_case.pattern, match_case.expression);
            }
            return format(" }}");
        }
        auto operator()(hir::expression::Ret const& ret) {
            return format("ret {}", ret.expression);
        }
        auto operator()(hir::expression::Size_of const& size_of) {
            return format("size_of({})", size_of.type);
        }
        auto operator()(hir::expression::Take_reference const& take) {
            return format("&{}{}", take.mutability, take.name);
        }
        auto operator()(hir::expression::Meta const& meta) {
            return format("meta {}", meta.expression);
        }
        auto operator()(hir::expression::Hole const&) {
            return format("???");
        }
    };


    struct Type_format_visitor : bu::fmt::Visitor_base {
        auto operator()(hir::type::Integer)   { return format("Int");    }
        auto operator()(hir::type::Floating)  { return format("Float");  }
        auto operator()(hir::type::Character) { return format("Char");   }
        auto operator()(hir::type::Boolean)   { return format("Bool");   }
        auto operator()(hir::type::String)    { return format("String"); }
        auto operator()(hir::type::Wildcard) {
            return format("_");
        }
        auto operator()(hir::type::Typename const& type) {
            return format("{}", type.identifier);
        }
        auto operator()(hir::type::Template_parameter_reference const& parameter) {
            return format("{}", parameter.name);
        }
        auto operator()(hir::type::Tuple const& tuple) {
            return format("({})", tuple.types);
        }
        auto operator()(hir::type::Array const& array) {
            return format("[{}; {}]", array.element_type, array.length);
        }
        auto operator()(hir::type::Slice const& slice) {
            return format("[{}]", slice.element_type);
        }
        auto operator()(hir::type::Function const& function) {
            return format("fn({}): {}", function.argument_types, function.return_type);
        }
        auto operator()(hir::type::Type_of const& type_of) {
            return format("type_of({})", type_of.expression);
        }
        auto operator()(hir::type::Reference const& reference) {
            return format("&{}{}", reference.mutability, reference.type);
        }
        auto operator()(hir::type::Pointer const& pointer) {
            return format("*{}{}", pointer.mutability, pointer.type);
        }
    };


    struct Pattern_format_visitor : bu::fmt::Visitor_base {
        template <class T>
        auto operator()(hir::pattern::Literal<T> const& literal) {
            return std::invoke(Expression_format_visitor { { out } }, hir::expression::Literal { literal.value });
        }
        auto operator()(hir::pattern::Wildcard const&) {
            return format("_");
        }
        auto operator()(hir::pattern::Name const& name) {
            return format("{}{}", name.mutability, name.identifier);
        }
        auto operator()(hir::pattern::Constructor const& ctor) {
            return format(ctor.pattern ? "ctor {}({})" : "ctor {}", ctor.name, ctor.pattern);
        }
        auto operator()(hir::pattern::Constructor_shorthand const& ctor) {
            return format(ctor.pattern ? ":{}({})" : ":{}", ctor.name, ctor.pattern);
        }
        auto operator()(hir::pattern::Tuple const& tuple) {
            return format("({})", tuple.patterns);
        }
        auto operator()(hir::pattern::Slice const& slice) {
            return format("[{}]", slice.patterns);
        }
        auto operator()(hir::pattern::As const& as) {
            return format("{} as {}{}", as.pattern, as.name.mutability, as.name.identifier);
        }
        auto operator()(hir::pattern::Guarded const& guarded) {
            return format("{} if {}", guarded.pattern, guarded.guard);
        }
    };


    struct Definition_format_visitor : bu::fmt::Visitor_base {
        auto operator()(hir::definition::Function const& function) {
            return format(
                "fn {}{}{}({}){} = {}",
                function.name,
                function.explicit_template_parameters,
                function.implicit_template_parameters,
                function.parameters,
                function.return_type.transform(": {}"_format).value_or(""),
                function.body
            );
        }
        auto operator()(auto const&) -> std::format_context::iterator {
            bu::todo();
        }
    };

}


DEFINE_FORMATTER_FOR(hir::Expression) {
    return std::visit(Expression_format_visitor { { context.out() } }, value.value);
}

DEFINE_FORMATTER_FOR(hir::Type) {
    return std::visit(Type_format_visitor { { context.out() } }, value.value);
}

DEFINE_FORMATTER_FOR(hir::Pattern) {
    return std::visit(Pattern_format_visitor { { context.out() } }, value.value);
}

DEFINE_FORMATTER_FOR(hir::Definition) {
    return std::visit(Definition_format_visitor { { context.out() } }, value.value);
}



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