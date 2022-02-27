#include "bu/utilities.hpp"
#include "ast_formatting.hpp"


DIRECTLY_DEFINE_FORMATTER_FOR(ast::Match::Case) {
    return std::format_to(context.out(), "{} -> {}", value.pattern, value.expression);
}

DIRECTLY_DEFINE_FORMATTER_FOR(ast::Function_argument) {
    return std::format_to(context.out(), value.name ? "{} = {}" : "{1}", value.name, value.expression);
}

DIRECTLY_DEFINE_FORMATTER_FOR(ast::Qualified_name) {
    auto out = context.out();

    std::visit(bu::Overload {
        [   ](std::monostate)              {},
        [out](ast::Root_qualifier::Global) { std::format_to(out, "::"        ); },
        [out](auto const& root)            { std::format_to(out, "{}::", root); }
    }, value.root_qualifier->value);

    for (auto& qualifier : value.qualifiers) {
        std::visit(bu::Overload {
            [out](ast::Middle_qualifier::Lower const& lower) {
                std::format_to(out, "{}::", lower.name);
            },
            [out](ast::Middle_qualifier::Upper const& upper) {
                std::format_to(
                    out,
                    upper.template_arguments ? "{}[{}]::" : "{}::",
                    upper.name,
                    upper.template_arguments
                );
            }
        }, qualifier.value);
    }

    return std::format_to(out, "{}", value.identifier);
}


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
        auto operator()(ast::Array_literal const& literal) {
            switch (literal.elements.size()) {
            case 0:
                return format("[;]");
            case 1:
                return format("[{};]", literal.elements.front());
            default:
                format("[{}", literal.elements.front());
                for (auto& element : literal.elements | std::views::drop(1)) {
                    format("; {}", element);
                }
                return format("]");
            }
        }
        auto operator()(ast::List_literal const& literal) {
            return format("[{}]", literal.elements);
        }
        auto operator()(lexer::Identifier const identifier) {
            return format("{}", identifier);
        }
        auto operator()(ast::Variable const& variable) {
            return format("{}", variable.name);
        }
        auto operator()(ast::Template_instantiation const& instantiation) {
            return format("{}[{}]", instantiation.name, instantiation.template_arguments);
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
            return format("{}.{}({})", invocation.expression, invocation.member_name, invocation.arguments);
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
        auto operator()(ast::Take_reference reference) {
            return format(reference.is_mutable ? "&mut {}" : "&{}", reference.expression);
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
        auto operator()(ast::pattern::Guarded const& guarded) {
            return format("{} if {}", guarded.pattern, guarded.guard);
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
        auto operator()(ast::type::Reference reference) {
            return format(reference.is_mutable ? "&mut {}" : "&{}", reference.type);
        }
        auto operator()(ast::type::Template_instantiation const& instantiation) {
            return format("{}[{}]", instantiation.name, instantiation.arguments);
        }
    };


    using Definition_variant = std::variant<
        ast::definition::Function          const*,
        ast::definition::Struct            const*,
        ast::definition::Data              const*,
        ast::definition::Alias             const*,
        ast::definition::Function_template const*,
        ast::definition::Struct_template   const*,
        ast::definition::Data_template     const*,
        ast::definition::Alias_template    const*
    >;

    auto format_definition(
        std::format_context&                        context,
        Definition_variant                          definition,
        std::vector<ast::Template_parameter> const* parameters = nullptr
    )
        -> std::format_context::iterator
    {
        auto const template_parameters = [&]() noexcept -> std::string {
            return parameters ? std::format("[{}]", *parameters) : "";
        };
        auto const format = [out = context.out()](std::string_view fmt, auto const&... args){
            return std::format_to(out, fmt, args...);
        };

        return std::visit(bu::Overload {
            [&](ast::definition::Function const* function) {
                return format(
                    "fn {}{}({}): {} = {}",
                    function->name,
                    template_parameters(),
                    function->parameters,
                    function->return_type,
                    function->body
                );
            },
            [&](ast::definition::Struct const* structure) {
                return format(
                    "struct {}{} = {}",
                    structure->name,
                    template_parameters(),
                    structure->members
                );
            },
            [&](ast::definition::Data const* data) {
                return format(
                    "data {}{} = {}",
                    data->name,
                    template_parameters(),
                    data->constructors
                );
            },
            [&](ast::definition::Alias const* alias) {
                return format(
                    "alias {}{} = {}",
                    alias->name,
                    template_parameters(),
                    alias->type
                );
            },
            [&](auto const* pointer) { // This overload catches the template versions
                return format_definition(context, &pointer->definition, &pointer->parameters);
            }
        }, definition);
    }

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


DIRECTLY_DEFINE_FORMATTER_FOR(ast::definition::Function::Parameter) {
    return std::format_to(context.out(), "{}: {} = {}", value.pattern, value.type, value.default_value);
}

DEFINE_FORMATTER_FOR(ast::definition::Function) {
    return format_definition(context, &value);
}
DEFINE_FORMATTER_FOR(ast::definition::Function_template) {
    return format_definition(context, &value);
}


DIRECTLY_DEFINE_FORMATTER_FOR(ast::definition::Struct::Member) {
    return std::format_to(context.out(), "{}: {}", value.name, value.type);
}

DEFINE_FORMATTER_FOR(ast::definition::Struct) {
    return format_definition(context, &value);
}
DEFINE_FORMATTER_FOR(ast::definition::Struct_template) {
    return format_definition(context, &value);
}


DIRECTLY_DEFINE_FORMATTER_FOR(ast::definition::Data::Constructor) {
    return std::format_to(context.out(), "{}({})", value.name, value.type);
}

DEFINE_FORMATTER_FOR(ast::definition::Data) {
    return format_definition(context, &value);
}
DEFINE_FORMATTER_FOR(ast::definition::Data_template) {
    return format_definition(context, &value);
}


DEFINE_FORMATTER_FOR(ast::definition::Alias) {
    return format_definition(context, &value);
}
DEFINE_FORMATTER_FOR(ast::definition::Alias_template) {
    return format_definition(context, &value);
}


DEFINE_FORMATTER_FOR(ast::definition::Typeclass) {
    auto out = context.out();
    std::format_to(out, value.self_is_template ? "class {} [] {{\n" : "class {} {{\n", value.name);

    for (auto& signature : value.function_signatures) {
        std::format_to(
            out,
            "fn {}[{}]({}): {}\n",
            signature.name,
            signature.template_parameters,
            signature.type.argument_types,
            signature.type.return_type
        );
    }
    for (auto& signature : value.type_signatures) {
        std::format_to(
            out,
            "alias {}[{}]: {}\n",
            signature.name,
            signature.template_parameters,
            signature.classes
        );
    }

    return std::format_to(out, "}}");
}


DIRECTLY_DEFINE_FORMATTER_FOR(ast::Class_reference) {
    return value.template_arguments
        ? std::format_to(context.out(), "{}[{}]", value.name, value.template_arguments)
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


DEFINE_FORMATTER_FOR(ast::Namespace) {
    auto out = context.out();
    std::format_to(out, "module {} {{", value.name);

    auto fmt = [out](auto const& xs) {
        for (auto& x : xs.span() | std::views::transform(bu::second)) {
            std::format_to(out, "\n{}", x);
        }
    };

    fmt(value.function_definitions);
    fmt(value.function_template_definitions);

    fmt(value.struct_definitions);
    fmt(value.struct_template_definitions);

    fmt(value.data_definitions);
    fmt(value.data_template_definitions);

    fmt(value.alias_definitions);
    fmt(value.alias_template_definitions);

    fmt(value.class_definitions);

    fmt(value.children);

    return std::format_to(out, "\n}}\n");
}