#include "bu/utilities.hpp"
#include "ast_formatting.hpp"


DIRECTLY_DEFINE_FORMATTER_FOR(ast::Match::Case) {
    return std::format_to(context.out(), "{} -> {}", value.pattern, value.expression);
}

DIRECTLY_DEFINE_FORMATTER_FOR(ast::Function_argument) {
    if (value.name) {
        return std::format_to(context.out(), "{} = {}", value.name, value.expression);
    }
    else {
        return std::format_to(context.out(), "{}", value.expression);
    }
}

DIRECTLY_DEFINE_FORMATTER_FOR(ast::Mutability) {
    switch (value.type) {
    case ast::Mutability::Type::mut:
        return std::format_to(context.out(), "mut ");
    case ast::Mutability::Type::immut:
        return context.out();
    default:
        return std::format_to(context.out(), "mut?{} ", value.parameter_name);
    }
}


DIRECTLY_DEFINE_FORMATTER_FOR(ast::definition::Function::Parameter) {
    return std::format_to(context.out(), "{}: {} = {}", value.pattern, value.type, value.default_value);
}

DIRECTLY_DEFINE_FORMATTER_FOR(ast::definition::Struct::Member) {
    return std::format_to(context.out(), "{}: {}", value.name, value.type);
}

DIRECTLY_DEFINE_FORMATTER_FOR(ast::definition::Data::Constructor) {
    return std::format_to(context.out(), "{}({})", value.name, value.type);
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
                std::format_to(context.out(), "{}", parameter.name);

                if (!parameter.classes.empty()) {
                    std::format_to(context.out(), ": ");
                    bu::format_delimited_range(context.out(), parameter.classes, " + ");
                }

                return context.out();
            },
            [&](ast::Template_parameter::Value_parameter const& parameter) {
                return std::format_to(context.out(), "{}: {}", parameter.name, parameter.type);
            },
            [&](ast::Template_parameter::Mutability_parameter const& parameter) {
                return std::format_to(context.out(), "{}: mut", parameter.name);
            }
        },
        value.value
    );
}

DIRECTLY_DEFINE_FORMATTER_FOR(ast::Template_argument) {
    return std::visit(bu::Overload {
        [&](ast::Mutability const& mutability) {
            switch (mutability.type) {
            case ast::Mutability::Type::mut:
                return std::format_to(context.out(), "mut");
            case ast::Mutability::Type::immut:
                return std::format_to(context.out(), "immut");
            default:
                return std::format_to(context.out(), "mut?{}", mutability.parameter_name);
            }
        },
        [&](auto const& argument) {
            return std::format_to(context.out(), "{}", argument);
        }
    }, value.value);
}


DEFINE_FORMATTER_FOR(ast::Qualified_name) {
    auto out = context.out();

    std::visit(bu::Overload {
        [   ](std::monostate)              {},
        [out](ast::Root_qualifier::Global) { std::format_to(out, "::"        ); },
        [out](auto const& root)            { std::format_to(out, "{}::", root); }
    }, value.root_qualifier->value);

    for (auto& qualifier : value.middle_qualifiers) {
        std::visit(bu::Overload {
            [out](ast::Qualifier::Lower const& lower) {
                std::format_to(out, "{}::", lower.name);
            },
            [out](ast::Qualifier::Upper const& upper) {
                if (upper.template_arguments) {
                    std::format_to(out, "{}[{}]::", upper.name, *upper.template_arguments);
                }
                else {
                    std::format_to(out, "{}::", upper.name);
                }
            }
        }, qualifier.value);
    }

    return std::format_to(out, "{}", value.primary_qualifier.identifier);
}


namespace {

    struct Visitor_base {
        std::format_context::iterator out;

        auto format(std::string_view fmt, auto const&... args) {
            return std::vformat_to(out, fmt, std::make_format_args(args...));
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
        auto operator()(ast::Take_reference const& reference) {
            return format("&{}{}", reference.mutability, reference.expression);
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
        auto operator()(ast::pattern::Literal<T> const& literal) {
            return std::invoke(Expression_format_visitor { { out } }, literal);
        }
        auto operator()(ast::pattern::Name const& name) {
            return format("{}{}", name.mutability, name.identifier);
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
        auto operator()(ast::type::Reference const& reference) {
            return format("&{}{}", reference.mutability, reference.type);
        }
        auto operator()(ast::type::Pointer const& pointer) {
            return format("*{}{}", pointer.mutability, pointer.type);
        }
        auto operator()(ast::type::Template_instantiation const& instantiation) {
            return format("{}[{}]", instantiation.name, instantiation.arguments);
        }

        auto operator()(ast::type::Inference_variable const& variable) {
            return format("${}", variable.tag);
        }
    };


    struct Definition_format_visitor : Visitor_base {
        std::vector<ast::Template_parameter> const* parameters;

        auto template_parameters() noexcept -> std::string {
            return parameters ? std::format("[{}]", *parameters) : "";
        }

        auto operator()(ast::definition::Function const& function) {
            return format(
                "fn {}{}({}){} = {}",
                function.name,
                template_parameters(),
                function.parameters,
                function.return_type ? std::format(": {}", *function.return_type) : "",
                function.body
            );
        }

        auto operator()(ast::definition::Struct const& structure) {
            return format(
                "struct {}{} = {}",
                structure.name,
                template_parameters(),
                structure.members
            );
        }

        auto operator()(ast::definition::Data const& data) {
            return format(
                "data {}{} = {}",
                data.name,
                template_parameters(),
                data.constructors
            );
        }

        auto operator()(ast::definition::Alias const& alias) {
            return format(
                "alias {}{} = {}",
                alias.name,
                template_parameters(),
                alias.type
            );
        }

        auto operator()(ast::definition::Typeclass const& typeclass) {
            format(
                "class {}{} {{",
                typeclass.name,
                template_parameters()
            );

            for (auto& signature : typeclass.function_signatures) {
                format(
                    "fn {}[{}]({}): {}\n",
                    signature.name,
                    signature.template_parameters,
                    signature.type.argument_types,
                    signature.type.return_type
                );
            }
            for (auto& signature : typeclass.type_signatures) {
                format(
                    "alias {}[{}]: {}\n",
                    signature.name,
                    signature.template_parameters,
                    signature.classes
                );
            }

            return format("}}");
        }

        auto operator()(ast::definition::Instantiation const& instantiation) {
            format("inst {} {} {{\n", instantiation.typeclass, instantiation.instance);
            bu::format_delimited_range(out, instantiation.definitions, "\n\n");
            return format("}}");
        }

        auto operator()(ast::definition::Implementation const& implementation) {
            format("impl {} {{\n", implementation.type);
            bu::format_delimited_range(out, implementation.definitions, "\n\n");
            return format("}}");
        }

        auto operator()(ast::definition::Namespace const& space) {
            format("namespace {} {{\n", space.name);
            bu::format_delimited_range(out, space.definitions, "\n\n");
            return format("\n}}");
        }

        template <class T>
        auto operator()(ast::definition::Template_definition<T> const& template_definition) {
            parameters = &template_definition.parameters;
            return operator()(template_definition.definition);
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

DEFINE_FORMATTER_FOR(ast::Definition) {
    return std::visit(Definition_format_visitor { { context.out() }, nullptr }, value.value);
}


DEFINE_FORMATTER_FOR(ast::Module) {
    if (value.name) {
        std::format_to(context.out(), "module {}\n", *value.name);
    }

    for (auto& import_ : value.imports) {
        std::format_to(context.out(), "import ");
        bu::format_delimited_range(context.out(), import_.path, ".");
        if (import_.alias) {
            std::format_to(context.out(), " as {}", *import_.alias);
        }
        std::format_to(context.out(), "\n");
    }

    return bu::format_delimited_range(context.out(), value.definitions, "\n\n");
}