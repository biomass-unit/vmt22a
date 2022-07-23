#include "bu/utilities.hpp"
#include "ast/ast.hpp"
#include "mir/mir.hpp"


DIRECTLY_DEFINE_FORMATTER_FOR(mir::Template_parameter_set) {
    std::ignore = value;
    std::ignore = context;
    bu::todo();
}


DEFINE_FORMATTER_FOR(mir::Class_reference) {
    std::ignore = value;
    std::ignore = context;
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
        auto operator()(auto const&) -> std::format_context::iterator {
            bu::todo();
        }
    };

    struct Pattern_format_visitor : bu::fmt::Visitor_base {
        auto operator()(auto const&) -> std::format_context::iterator {
            bu::todo();
        }
    };

    struct Type_format_visitor : bu::fmt::Visitor_base {
        auto operator()(mir::type::Integer const integer) {
            return format([=] {
                using enum mir::type::Integer;
                switch (integer) {
                case i8:  return "i8";
                case i16: return "i16";
                case i32: return "i32";
                case i64: return "i64";
                case u8:  return "u8";
                case u16: return "u16";
                case u32: return "u32";
                case u64: return "u64";
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
        auto operator()(mir::type::General_variable const& variable) {
            return format("'T{}", variable.tag.value);
        }
        auto operator()(mir::type::Integral_variable const& variable) {
            return format("'I{}", variable.tag.value);
        }
        auto operator()(mir::type::Parameterized const& parameterized) {
            return format("(\\{} -> {})", parameterized.parameters, parameterized.body);
        }
    };

}


DEFINE_FORMATTER_FOR(mir::Expression) {
    return std::visit(Expression_format_visitor { { context.out() } }, value.value);
}

DEFINE_FORMATTER_FOR(mir::Pattern) {
    return std::visit(Pattern_format_visitor { { context.out() } }, value.value);
}

DEFINE_FORMATTER_FOR(mir::Type) {
    return std::visit(Type_format_visitor { { context.out() } }, value.value);
}