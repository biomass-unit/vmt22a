#include "bu/utilities.hpp"
#include "ast_formatting.hpp"


DIRECTLY_DEFINE_FORMATTER_FOR(ast::expression::Match::Case) {
    return std::format_to(context.out(), "{} -> {}", value.pattern, value.expression);
}

DIRECTLY_DEFINE_FORMATTER_FOR(ast::expression::Lambda::Capture) {
    return std::visit(bu::Overload {
        [&](ast::expression::Lambda::Capture::By_pattern const& capture) {
            return std::format_to(context.out(), "{} = {}", capture.pattern, capture.expression);
        },
        [&](ast::expression::Lambda::Capture::By_reference const& capture) {
            return std::format_to(context.out(), "&{}", capture.variable);
        }
    }, value.value);
}

DIRECTLY_DEFINE_FORMATTER_FOR(ast::Function_argument) {
    if (value.name) {
        std::format_to(context.out(), "{} = ", *value.name);
    }
    return std::format_to(context.out(), "{}", value.expression);
}

DIRECTLY_DEFINE_FORMATTER_FOR(ast::Mutability) {
    switch (value.type) {
    case ast::Mutability::Type::mut:
        return std::format_to(context.out(), "mut ");
    case ast::Mutability::Type::immut:
        return context.out();
    case ast::Mutability::Type::parameterized:
        return std::format_to(context.out(), "mut?{} ", value.parameter_name);
    default:
        std::unreachable();
    }
}


DIRECTLY_DEFINE_FORMATTER_FOR(ast::Function_parameter) {
    std::format_to(context.out(), "{}", value.pattern);
    if (value.type) {
        std::format_to(context.out(), ": {}", *value.type);
    }
    if (value.default_value) {
        std::format_to(context.out(), " = {}", *value.default_value);
    }
    return context.out();
}

DIRECTLY_DEFINE_FORMATTER_FOR(ast::definition::Struct::Member) {
    return std::format_to(
        context.out(),
        "{}{}: {}",
        value.is_public ? "pub " : "",
        value.name,
        value.type
    );
}

DIRECTLY_DEFINE_FORMATTER_FOR(ast::definition::Enum::Constructor) {
    return std::format_to(context.out(), "{}({})", value.name, value.type);
}


DIRECTLY_DEFINE_FORMATTER_FOR(ast::Class_reference) {
    return value.template_arguments
        ? std::format_to(context.out(), "{}[{}]", value.name, value.template_arguments)
        : std::format_to(context.out(), "{}", value.name);
}

DIRECTLY_DEFINE_FORMATTER_FOR(ast::Template_parameter) {
    std::format_to(context.out(), "{}", value.name);

    return std::visit(
        bu::Overload {
            [&](ast::Template_parameter::Type_parameter const& parameter) {
                if (!parameter.classes.empty()) {
                    std::format_to(
                        context.out(),
                        ": {}",
                        bu::fmt::delimited_range(parameter.classes, " + ")
                    );
                }
                return context.out();
            },
            [&](ast::Template_parameter::Value_parameter const& parameter) {
                return parameter.type
                    ? std::format_to(context.out(), ": {}", *parameter.type)
                    : context.out();
            },
            [&](ast::Template_parameter::Mutability_parameter const&) {
                return std::format_to(context.out(), ": mut");
            }
        },
        value.value
    );
}

DIRECTLY_DEFINE_FORMATTER_FOR(ast::Template_argument) {
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
        [&]<bu::one_of<ast::Type, ast::Expression> T>(T const& argument) {
            return std::format_to(context.out(), "{}", argument);
        }
    }, value.value);
}


DEFINE_FORMATTER_FOR(ast::Name) {
    return std::format_to(context.out(), "{}", value.identifier);
}

DEFINE_FORMATTER_FOR(ast::Qualified_name) {
    auto out = context.out();

    std::visit(bu::Overload {
        [   ](std::monostate)              {},
        [out](ast::Root_qualifier::Global) { std::format_to(out, "::"        ); },
        [out](auto const& root)            { std::format_to(out, "{}::", root); }
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


namespace {

    struct Visitor_base {
        std::format_context::iterator out;

        auto format(std::string_view const fmt, auto const&... args) {
            return std::vformat_to(out, fmt, std::make_format_args(args...));
        }
    };


    struct Expression_format_visitor : Visitor_base {
        template <class T>
        auto operator()(ast::expression::Literal<T> const& literal) {
            return format("{}", literal.value);
        }
        auto operator()(ast::expression::Literal<char> const& literal) {
            return format("'{}'", literal.value);
        }
        auto operator()(ast::expression::Literal<lexer::String> const& literal) {
            return format("\"{}\"", literal.value);
        }
        auto operator()(ast::expression::Array_literal const& literal) {
            return format("[{}]", literal.elements);
        }
        auto operator()(ast::expression::Variable const& variable) {
            return format("{}", variable.name);
        }
        auto operator()(ast::expression::Dereference const& dereference) {
            return format("*{}", dereference.expression);
        }
        auto operator()(ast::expression::Template_instantiation const& instantiation) {
            return format("{}[{}]", instantiation.name, instantiation.template_arguments);
        }
        auto operator()(ast::expression::Tuple const& tuple) {
            return format("({})", tuple.expressions);
        }
        auto operator()(ast::expression::Invocation const& invocation) {
            return format("{}({})", invocation.invocable, invocation.arguments);
        }
        auto operator()(ast::expression::Struct_initializer const& initializer) {
            return format(
                "{} {{ {} }}",
                initializer.type,
                initializer.member_initializers.container()
            );
        }
        auto operator()(ast::expression::Binary_operator_invocation const& invocation) {
            return format("({} {} {})", invocation.left, invocation.op, invocation.right);
        }
        auto operator()(ast::expression::Member_access_chain const& chain) {
            format("({}", chain.expression);
            for (auto& accessor : chain.accessors) {
                format(".{}", accessor);
            }
            return format(")");
        }
        auto operator()(ast::expression::Member_function_invocation const& invocation) {
            return format("{}.{}({})", invocation.expression, invocation.member_name, invocation.arguments);
        }
        auto operator()(ast::expression::Block const& block) {
            format("{{ {}", bu::fmt::delimited_range(block.side_effects, "; "));
            return block.result ? format("; {} }}", *block.result) : format(" }}");
        }
        auto operator()(ast::expression::Conditional const& conditional) {
            auto& [condition, true_branch, false_branch] = conditional;
            return format("if {} {} else {}", condition, true_branch, false_branch);
        }
        auto operator()(ast::expression::Match const& match) {
            return format("match {} {{ {} }}", match.expression, match.cases);
        }
        auto operator()(ast::expression::Type_cast const& cast) {
            return format(
                "({} {} {})",
                cast.expression,
                [&] {
                    using enum ast::expression::Type_cast::Kind;

                    switch (cast.kind) {
                    case conversion:
                        return "as";
                    case ascription:
                        return ":";
                    default:
                        std::unreachable();
                    }
                }(),
                cast.target
            );
        }
        auto operator()(ast::expression::Let_binding const& binding) {
            return format("let {}: {} = {}", binding.pattern, binding.type, binding.initializer);
        }
        auto operator()(ast::expression::Conditional_let const& binding) {
            return format("let {} = {}", binding.pattern, binding.initializer);
        }
        auto operator()(ast::expression::Local_type_alias const& alias) {
            return format("alias {} = {}", alias.name, alias.type);
        }
        auto operator()(ast::expression::Lambda const& lambda) {
            return format(
                "\\{}{}{} -> {}",
                lambda.parameters,
                lambda.explicit_captures.empty() ? "" : " . ",
                lambda.explicit_captures,
                lambda.body
            );
        }
        auto operator()(ast::expression::Infinite_loop const& loop) {
            return format("loop {}", loop.body);
        }
        auto operator()(ast::expression::While_loop const& loop) {
            return format("while {} {}", loop.condition, loop.body);
        }
        auto operator()(ast::expression::For_loop const& loop) {
            return format("for {} in {} {}", loop.iterator, loop.iterable, loop.body);
        }
        auto operator()(ast::expression::Continue) {
            return format("continue");
        }
        auto operator()(ast::expression::Break const& break_) {
            return format("break {}", break_.expression);
        }
        auto operator()(ast::expression::Ret const& ret) {
            return format("ret {}", ret.expression);
        }
        auto operator()(ast::expression::Size_of const& size_of) {
            return format("size_of({})", size_of.type);
        }
        auto operator()(ast::expression::Take_reference const& reference) {
            return format("&{}{}", reference.mutability, reference.name);
        }
        auto operator()(ast::expression::Meta const& meta) {
            return format("meta({})", meta.expression);
        }
        auto operator()(ast::expression::Hole const&) {
            return format("???");
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
        auto operator()(ast::pattern::Constructor const& ctor) {
            return format(ctor.pattern ? "ctor {}({})" : "ctor {}", ctor.name, ctor.pattern);
        }
        auto operator()(ast::pattern::Constructor_shorthand const& ctor) {
            return format(ctor.pattern ? ":{}({})" : ":{}", ctor.name, ctor.pattern);
        }
        auto operator()(ast::pattern::Tuple const& tuple) {
            return format("({})", tuple.patterns);
        }
        auto operator()(ast::pattern::Slice const& slice) {
            return format("[{}]", slice.patterns);
        }
        auto operator()(ast::pattern::As const& as) {
            return format("{} as {}{}", as.pattern, as.name.mutability, as.name.identifier);
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

        auto operator()(ast::type::Wildcard const&) {
            return format("_");
        }
        auto operator()(ast::type::Typename const& name) {
            return format("{}", name.identifier);
        }
        auto operator()(ast::type::Tuple const& tuple) {
            return format("({})", tuple.types);
        }
        auto operator()(ast::type::Array const& array) {
            return format("[{}; {}]", array.element_type, array.length);
        }
        auto operator()(ast::type::Slice const& slice) {
            return format("[{}]", slice.element_type);
        }
        auto operator()(ast::type::Function const& function) {
            return format("fn({}): {}", function.argument_types, function.return_type);
        }
        auto operator()(ast::type::Type_of const& type_of) {
            return format("type_of({})", type_of.expression);
        }
        auto operator()(ast::type::Instance_of const& instance_of) {
            return format("inst {}", bu::fmt::delimited_range(instance_of.classes, " + "));
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

        auto operator()(ast::definition::Enum const& enumeration) {
            return format(
                "enum {}{} = {}",
                enumeration.name,
                template_parameters(),
                enumeration.constructors
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
            return format(
                "inst {} {} {{\n{}\n}}",
                instantiation.typeclass,
                instantiation.instance,
                bu::fmt::delimited_range(instantiation.definitions, "\n\n")
            );
        }

        auto operator()(ast::definition::Implementation const& implementation) {
            return format(
                "impl {} {{\n{}\n}}",
                implementation.type,
                bu::fmt::delimited_range(implementation.definitions, "\n\n")
            );
        }

        auto operator()(ast::definition::Namespace const& space) {
            return format(
                "namespace {}{} {{\n{}\n}}",
                space.name,
                template_parameters(),
                bu::fmt::delimited_range(space.definitions, "\n\n")
            );
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

    for (auto& import : value.imports) {
        std::format_to(context.out(), "import {}", bu::fmt::delimited_range(import.path.components, "."));
        if (import.alias) {
            std::format_to(context.out(), " as {}", *import.alias);
        }
        std::format_to(context.out(), "\n");
    }

    return std::format_to(context.out(), "{}", bu::fmt::delimited_range(value.definitions, "\n\n"));
}