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
        auto operator()(ast::Member_access const& expression) {
            return format("({}.{})", expression.expression, expression.member_name);
        }
        auto operator()(ast::Member_function_invocation const& invocation) {
            return format("{}.{}()", invocation.expression, invocation.member_name, invocation.arguments);
        }
        auto operator()(ast::Tuple_member_access const& expression) {
            return format("({}.{})", expression.expression, expression.member_index);
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
            return format("{} as {}", cast.expression, cast.target);
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
        auto operator()(ast::type::Integer)   { return format("Int");    }
        auto operator()(ast::type::Floating)  { return format("Float");  }
        auto operator()(ast::type::Character) { return format("Char");   }
        auto operator()(ast::type::Boolean)   { return format("Bool");   }
        auto operator()(ast::type::String)    { return format("String"); }

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


template <>
struct std::formatter<ast::definition::Function::Parameter> : bu::Formatter_base {
    auto format(auto const& parameter, std::format_context& context) {
        return std::format_to(context.out(), "{}: {}", parameter.pattern, parameter.type);
    }
};

DEFINE_FORMATTER_FOR(ast::definition::Function) {
    return std::format_to(
        context.out(),
        "fn {}({}): {} = {}",
        value.name,
        value.parameters,
        value.return_type,
        value.body
    );
}


template <>
struct std::formatter<ast::definition::Struct::Member> : bu::Formatter_base {
    auto format(auto const& member, std::format_context& context) {
        return std::format_to(context.out(), "{}: {}", member.name, member.type);
    }
};

DEFINE_FORMATTER_FOR(ast::definition::Struct) {
    return std::format_to(context.out(), "struct {} = {}", value.name, value.members);
}


template <>
struct std::formatter<ast::definition::Data::Constructor> : bu::Formatter_base {
    auto format(auto const& ctor, std::format_context& context) {
        return std::format_to(context.out(), "{}({})", ctor.name, ctor.type);
    }
};

DEFINE_FORMATTER_FOR(ast::definition::Data) {
    return std::format_to(context.out(), "data {} = {}", value.name, value.constructors);
}


DEFINE_FORMATTER_FOR(ast::definition::Typeclass) {
    auto out = context.out();
    std::format_to(out, "class {} {{");

    for (auto& signature : value.function_signatures) {
        std::format_to(
            out,
            "\nfn {}({}): {}",
            signature.name,
            signature.type.argument_types,
            signature.type.return_type
        );
    }
    for (auto& signature : value.type_signatures) {
        std::format_to(out, "\n{}", signature.name);
    }

    return std::format_to(out, "}}");
}


DIRECTLY_DEFINE_FORMATTER_FOR(ast::Class_reference) {
    return value.arguments
        ? std::format_to(context.out(), "{}[{}]", value.name, value.arguments)
        : std::format_to(context.out(), "{}", value.name);
}

DIRECTLY_DEFINE_FORMATTER_FOR(ast::Template_parameter) {
    return std::visit(
        bu::Overload {
            [&](ast::Template_parameter::Type_parameter const& parameter) {
                return parameter.classes.empty()
                    ? std::format_to(context.out(), "{}", parameter.name)
                    : std::format_to(context.out(), "{}: {}", parameter.name, parameter.classes);
            },
            [&](ast::Template_parameter::Value_parameter const& parameter) {
                return std::format_to(context.out(), "{}: {}", parameter.name, parameter.type);
            }
        },
        value.value
    );
}

DIRECTLY_DEFINE_FORMATTER_FOR(ast::Template_argument) {
    return std::format_to(context.out(), "{}", value.value);
}


template <class T>
DEFINE_FORMATTER_FOR(ast::definition::Template_definition<T>) {
    return std::format_to(context.out(), "template [{}] {}", value.parameters, value.definition);
}

template struct std::formatter<ast::definition::Function_template>;
template struct std::formatter<ast::definition::Struct_template>;
template struct std::formatter<ast::definition::Data_template>;


DEFINE_FORMATTER_FOR(ast::Namespace) {
    auto out = context.out();
    std::format_to(out, "module {} {{", value->name);

    auto fmt = [out]<class T>(std::vector<T> const& xs) {
        for (auto& x : xs) {
            std::format_to(out, "\n{}", x);
        }
    };

    fmt(value->function_definitions);
    fmt(value->function_template_definitions);
    fmt(value->struct_definitions);
    fmt(value->struct_template_definitions);
    fmt(value->data_definitions);
    fmt(value->data_template_definitions);
    fmt(value->children);

    return std::format_to(out, "\n}}\n");
}