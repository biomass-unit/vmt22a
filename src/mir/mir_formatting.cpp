#include "bu/utilities.hpp"
#include "ast/ast.hpp"
#include "mir/mir.hpp"

#include "resolution/resolution_internals.hpp" // FIX


DIRECTLY_DEFINE_FORMATTER_FOR(mir::Template_parameter_set) {
    (void)value;
    return context.out();
    /*(void)value;
    (void)context;
    bu::todo();*/
}

DIRECTLY_DEFINE_FORMATTER_FOR(mir::Function_parameter) {
    return std::format_to(context.out(), "{}: {}", value.pattern, value.type);
}

DIRECTLY_DEFINE_FORMATTER_FOR(mir::Struct::Member) {
    return std::format_to(context.out(), "{}{}: {}", value.is_public ? "pub " : "", value.name, value.type);
}

DIRECTLY_DEFINE_FORMATTER_FOR(mir::Enum::Constructor) {
    return std::format_to(context.out(), "{}{}", value.name, value.type.transform("({})"_format).value_or(""));
}


DEFINE_FORMATTER_FOR(mir::Class_reference) {
    (void)value;
    (void)context;
    bu::todo();
}


namespace {

    struct Expression_format_visitor : bu::fmt::Visitor_base {
        template <class T>
        auto operator()(mir::expression::Literal<T> const& literal) {
            return format("{}", literal.value);
        }
        auto operator()(mir::expression::Literal<char> const& literal) {
            return format("'{}'", literal.value);
        }
        auto operator()(mir::expression::Literal<lexer::String> const& literal) {
            return format("\"{}\"", literal.value);
        }
        auto operator()(mir::expression::Function_reference const& function) {
            return format("{}", std::visit([](auto& f) { return f.name; }, function.info->value)); // FIX
        }
        auto operator()(mir::expression::Tuple const& tuple) {
            return format("({})", tuple.elements);
        }
        auto operator()(mir::expression::Block const& block) {
            format("{{ ");
            for (auto const& side_effect : block.side_effects) {
                format("{}; ", side_effect);
            }
            return format("{}}}", block.result.transform("{} "_format).value_or(""));
        }
        auto operator()(mir::expression::Let_binding const& let) {
            return format("let {}: {} = {}", let.pattern, let.type, let.initializer);
        }
        auto operator()(mir::expression::Array_literal const& array) {
            return format("[{}]", array.elements);
        }
        auto operator()(mir::expression::Local_variable_reference const& variable) {
            return format("{}", variable.name);
        }
    };

    struct Pattern_format_visitor : bu::fmt::Visitor_base {
        auto operator()(mir::pattern::Wildcard const&) {
            return format("_");
        }
        auto operator()(mir::pattern::Name const& name) {
            return format("{}{}", name.is_mutable ? "mut " : "", name.value);
        }
        auto operator()(mir::pattern::Tuple const& tuple) {
            return format("({})", tuple.patterns);
        }
        auto operator()(mir::pattern::Slice const& slice) {
            return format("[{}]", slice.patterns);
        }
        auto operator()(mir::pattern::As const& as) {
            format("{} as", as.pattern);
            return operator()(as.name);
        }
        auto operator()(mir::pattern::Guarded const& guarded) {
            return format("{} if {}", guarded.pattern, guarded.guard);
        }
        auto operator()(mir::pattern::Enum_constructor const& ctor) {
            return format(
                "{}{}",
                ctor.constructor.name,
                ctor.pattern.transform("({})"_format).value_or("")
            );
        }
    };

    struct Type_format_visitor : bu::fmt::Visitor_base {
        auto operator()(mir::type::Integer const integer) {
            return format([=] {
                using enum mir::type::Integer;
                switch (integer) {
                case i8:  return "I8";
                case i16: return "I16";
                case i32: return "I32";
                case i64: return "I64";
                case u8:  return "U8";
                case u16: return "U16";
                case u32: return "U32";
                case u64: return "U64";
                default:
                    std::unreachable();
                }
            }());
        }
        auto operator()(mir::type::Floating)  { return format("Float"); }
        auto operator()(mir::type::Character) { return format("Char"); }
        auto operator()(mir::type::Boolean)   { return format("Bool"); }
        auto operator()(mir::type::String)    { return format("String"); }
        auto operator()(mir::type::Array const& array) {
            return format("[{}; {}]", array.element_type, array.length);
        }
        auto operator()(mir::type::Slice const& slice) {
            return format("[{}]", slice.element_type);
        }
        auto operator()(mir::type::Reference const& reference) {
            return format("&{}{}", reference.mutability, reference.referenced_type);
        }
        auto operator()(mir::type::Function const& function) {
            return format("fn({}): {}", function.arguments, function.return_type);
        }
        auto operator()(mir::type::Tuple const& tuple) {
            return format("({})", tuple.types);
        }
        auto operator()(mir::type::Structure const& structure) {
            return std::visit(bu::Overload {
                [this](hir::definition::Struct& structure) {
                    return format("{}", structure.name);
                },
                [this](mir::Struct& structure) {
                    return format("{}", structure.name);
                }
            }, structure.info->value);
        }
        auto operator()(mir::type::Enumeration const&) -> std::format_context::iterator {
            bu::todo();
        }
        auto operator()(mir::type::Parameterized const& parameterized) {
            return format("(\\{} -> {})", parameterized.parameters, parameterized.body);
        }
        auto operator()(mir::type::General_variable const& variable) {
            return format("'T{}", variable.tag.value);
        }
        auto operator()(mir::type::Integral_variable const& variable) {
            return format("'I{}", variable.tag.value);
        }
    };

}


DEFINE_FORMATTER_FOR(mir::Expression) {
    std::visit(Expression_format_visitor { { context.out() } }, value.value);
    return std::format_to(context.out(), ": {}", value.type);
}

DEFINE_FORMATTER_FOR(mir::Pattern) {
    return std::visit(Pattern_format_visitor { { context.out() } }, value.value);
}

DEFINE_FORMATTER_FOR(mir::Type) {
    return std::visit(Type_format_visitor { { context.out() } }, value.value);
}


DEFINE_FORMATTER_FOR(mir::Function) {
    return std::format_to(
        context.out(),
        "fn {}{}({}): {} = {}",
        value.name,
        value.signature.template_parameters,
        value.signature.parameters,
        value.signature.return_type,
        value.body
    );
}

DEFINE_FORMATTER_FOR(mir::Struct) {
    return std::format_to(
        context.out(),
        "struct {}{} = {}",
        value.name,
        value.template_parameters,
        value.members
    );
}

DEFINE_FORMATTER_FOR(mir::Enum) {
    return std::format_to(
        context.out(),
        "enum {}{} = {}",
        value.name,
        value.template_parameters,
        bu::fmt::delimited_range(value.constructors, " | ")
    );
}

DEFINE_FORMATTER_FOR(mir::Alias) {
    return std::format_to(
        context.out(),
        "alias {}{} = {}",
        value.name,
        value.template_parameters,
        value.aliased_type
    );
}

DEFINE_FORMATTER_FOR(mir::Typeclass) {
    (void)context;
    (void)value;
    bu::todo();
}