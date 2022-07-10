#include "bu/utilities.hpp"
#include "hir.hpp"


DIRECTLY_DEFINE_FORMATTER_FOR(hir::Function_parameter) {
    return std::format_to(
        context.out(),
        "{}{}{}",
        value.pattern,
        value.type.transform(": {}"_format).value_or(""),
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
        auto operator()(hir::expression::Tuple const& tuple) {
            return format("({})", tuple.elements);
        }
        auto operator()(hir::expression::Conditional const& conditional) {
            return format("if {} {} else {}", conditional.condition, conditional.true_branch, conditional.false_branch);
        }
        auto operator()(hir::expression::Loop const& loop) {
            return format("loop { {} }", loop.body);
        }
        auto operator()(hir::expression::Break const&) {
            return format("break");
        }
        auto operator()(hir::expression::Block const& block) {
            format("{{");
            for (auto const& side_effect : block.side_effects) {
                format("{}; ", side_effect);
            }
            return format("{} }}", block.result);
        }
        auto operator()(hir::expression::Invocation const& invocation) {
            return format("{}({})", invocation.invocable, invocation.arguments);
        }
        auto operator()(hir::expression::Match const& match) {
            format("match {} {{ ", match.expression);
            for (auto& match_case : match.cases) {
                format("{} -> {}", match_case.pattern, match_case.expression);
            }
            return format(" }}");
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
                "fn {}{}({}){} {{ {} }}",
                function.name,
                function.explicit_template_parameters,
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