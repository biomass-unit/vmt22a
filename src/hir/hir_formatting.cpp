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

DIRECTLY_DEFINE_FORMATTER_FOR(hir::Implicit_template_parameter::Tag) {
    return std::format_to(context.out(), "X#{}", value.value);
}

DIRECTLY_DEFINE_FORMATTER_FOR(hir::Implicit_template_parameter) {
    return std::vformat_to(
        context.out(),
        value.classes.empty() ? "{}" : "{}: {}",
        std::make_format_args(
            value.tag,
            bu::fmt::delimited_range(value.classes, " + ")
        )
    );
}


namespace {

    struct Expression_format_visitor : bu::fmt::Visitor_base {
        template <class T>
        auto operator()(hir::expression::Literal<T> const& literal) {
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
        auto operator()(hir::expression::Loop const& loop) {
            return format("loop {{ {} }}", loop.body);
        }
        auto operator()(hir::expression::Break const& break_) {
            format("break");
            if (break_.label.has_value()) {
                format(" {} loop", *break_.label);
            }
            if (break_.expression.has_value()) {
                format(" {}", *break_.expression);
            }
            return out;
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
        auto operator()(hir::expression::Member_access_chain const& chain) {
            return format("({}.{})", chain.expression, bu::fmt::delimited_range(chain.accessors, "."));
        }
        auto operator()(hir::expression::Member_function_invocation const& invocation) {
            return format("{}.{}({})", invocation.expression, invocation.member_name, invocation.arguments);
        }
        auto operator()(hir::expression::Match const& match) {
            format("match {} {{ ", match.expression);
            for (auto& match_case : match.cases) {
                format("{} -> {}", match_case.pattern, match_case.expression);
            }
            return format(" }}");
        }
        auto operator()(hir::expression::Dereference const& deref) {
            return format("(*{})", deref.expression);
        }
        auto operator()(hir::expression::Template_application const& application) {
            return format("{}[{}]", application.name, application.template_arguments);
        }
        auto operator()(hir::expression::Type_cast const& cast) {
            return format("({} {} {})", cast.expression, cast.kind, cast.target);
        }
        auto operator()(hir::expression::Let_binding const& let) {
            return format(
                "let {}{} = {}",
                let.pattern,
                let.type.transform(": {}"_format).value_or(""),
                let.initializer
            );
        }
        auto operator()(hir::expression::Local_type_alias const& alias) {
            return format("alias {} = {}", alias.name, alias.type);
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
        auto operator()(hir::type::Implicit_parameter_reference const& parameter) {
            return format("{}", parameter.tag);
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
        auto operator()(hir::type::Instance_of const& instance_of) {
            return format("inst {}", bu::fmt::delimited_range(instance_of.classes, " + "));
        }
        auto operator()(hir::type::Template_application const& application) {
            return format("{}[{}]", application.name, application.arguments);
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
                function.implicit_template_parameters.empty()
                    ? ""
                    : "[{}]"_format(function.implicit_template_parameters),
                function.parameters,
                function.return_type.transform(": {}"_format).value_or(""),
                function.body
            );
        }
        auto operator()(auto const& definition) {
            // Non-function definitions share formatters with AST definitions
            return format("{}", definition);
        }
    };

}


DEFINE_FORMATTER_FOR(hir::Expression) {
    std::visit(Expression_format_visitor { { context.out() } }, value.value);
    return std::format_to(context.out(), ": {}", value.type);
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