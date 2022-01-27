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
    };


    struct Expression_format_visitor : Visitor_base {
        template <class T>
        auto operator()(ast::Literal<T> const& literal) {
            return format("{}", literal.value);
        }
        auto operator()(ast::Literal<char> const literal) {
            return format("'{}'", literal.value);
        }
        auto operator()(ast::Literal<lexer::String> const literal) {
            return format("\"{}\"", literal.value);
        }
        auto operator()(lexer::Identifier const identifier) {
            return format("{}", identifier);
        }
        auto operator()(ast::Variable variable) {
            return format("{}", variable.name);
        }
        auto operator()(ast::Tuple const& tuple) {
            return format("({})", tuple.expressions);
        }
        auto operator()(ast::Invocation const& invocation) {
            return format("{}({})", invocation.invocable, invocation.arguments);
        }
        auto operator()(ast::Binary_operator_invocation const& invocation) {
            return format("({} {} {})", invocation.left, invocation.op, invocation.right);
        }
        auto operator()(ast::Compound_expression const& compound) {
            return format("{{ {} }}", compound.expressions);
        }
        auto operator()(ast::Conditional const& conditional) {
            auto& [condition, true_branch, false_branch] = conditional;
            return format("if {} {} else {}", condition, true_branch, false_branch);
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
        auto operator()(ast::Infinite_loop const& loop) {
            return format("loop {}", loop.body);
        }
        auto operator()(ast::While_loop const& loop) {
            return format("while {} {}", loop.condition, loop.body);
        }
        auto operator()(ast::For_loop const& loop) {
            return format("for {} in {} {}", loop.iterator, loop.iterable, loop.body);
        }
        auto operator()(ast::Continue) {
            return format("continue");
        }
        auto operator()(ast::Break const& break_) {
            return format("break {}", break_.expression);
        }
        auto operator()(ast::Ret const& ret) {
            return format("ret {}", ret.expression);
        }
        auto operator()(ast::Size_of const& size_of) {
            return format("size_of({})", size_of.type);
        }
        auto operator()(ast::Meta const& meta) {
            return format("meta({})", meta.expression);
        }
    };

    struct Pattern_format_visitor : Visitor_base {
        auto operator()(ast::pattern::Wildcard) {
            return format("_");
        }
        template <class T>
        auto operator()(ast::pattern::Literal<T> literal) {
            return std::invoke(Expression_format_visitor { { out } }, literal);
        }
        auto operator()(ast::pattern::Name name) {
            return format("{}", name.identifier);
        }
        auto operator()(ast::pattern::Tuple const& tuple) {
            return format("({})", tuple.patterns);
        }
    };

    struct Type_format_visitor : Visitor_base {
        auto operator()(ast::type::Int)    { return format("Int");    }
        auto operator()(ast::type::Float)  { return format("Float");  }
        auto operator()(ast::type::Char)   { return format("Char");   }
        auto operator()(ast::type::Bool)   { return format("Bool");   }
        auto operator()(ast::type::String) { return format("String"); }

        auto operator()(ast::type::Typename name) {
            return format("{}", name.identifier);
        }
        auto operator()(ast::type::Tuple const& tuple) {
            return format("({})", tuple.types);
        }
        auto operator()(ast::type::Array const& array) {
            return format("[{}; {}]", array.element_type, array.length);
        }
        auto operator()(ast::type::List const& list) {
            return format("[{}]", list.element_type);
        }
        auto operator()(ast::type::Function const& function) {
            return format("fn({}): {}", function.argument_types, function.return_type);
        }
        auto operator()(ast::type::Type_of const& type_of) {
            return format("type_of({})", type_of.expression);
        }
    };

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