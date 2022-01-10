#include "bu/utilities.hpp"
#include "ast_formatting.hpp"


template <>
struct std::formatter<ast::Match::Case> : bu::Formatter_base {
    auto format(ast::Match::Case const& match_case, std::format_context& context) {
        return std::format_to(context.out(), "{} -> {}", match_case.pattern, match_case.expression);
    }
};


namespace {

    struct Visitor_base {
        std::format_context::iterator out;
        auto format(std::string_view fmt, auto const&... args) {
            return std::format_to(out, fmt, args...);
        }
        auto operator()(auto const&) -> std::format_context::iterator {
            bu::unimplemented();
        }
    };


    struct Expression_format_visitor : Visitor_base {
        template <class T>
        auto operator()(ast::Literal<T> const& literal) {
            return format("{}", literal.value);
        }
        auto operator()(ast::Tuple const& tuple) {
            return format("({})", tuple.expressions);
        }
        auto operator()(ast::Invocation const& invocation) {
            return format("{}({})", invocation.invocable, invocation.arguments);
        }
        auto operator()(ast::Compound_expression const& compound) {
            return format("{{ {} }}", compound.expressions);
        }
        auto operator()(ast::Conditional const& conditional) {
            auto& [condition, true_branch, false_branch] = conditional;
            return format("if {} {{ {} }} else {{ {} }}", condition, true_branch, false_branch);
        }
        auto operator()(ast::Match const& match) {
            return format("match {} {{ {} }}", match.expression, match.cases);
        }
        auto operator()(ast::Type_cast const& cast) {
            return format("({}) as {}", cast.expression, cast.target);
        }
        auto operator()(ast::Let_binding const& binding) {
            return format("let {}: {} = {}", binding.pattern, binding.type, binding.initializer);
        }
    };

    struct Pattern_format_visitor : Visitor_base {};

    struct Type_format_visitor : Visitor_base {};

    struct Definition_format_visitor : Visitor_base {};

}


DEFINE_FORMATTER_FOR(ast::Expression) {
    return std::visit(Expression_format_visitor { { context.out() } }, value.value);
}

DEFINE_FORMATTER_FOR(ast::Pattern) {
    return std::visit(Pattern_format_visitor { { context.out() } }, value.value);
}

DEFINE_FORMATTER_FOR(ast::Type) {
    return std::visit(Type_format_visitor { { context.out() } }, value.value);
}

DEFINE_FORMATTER_FOR(ast::Definition) {
    std::ignore = value, std::ignore = context;
    bu::unimplemented();
}