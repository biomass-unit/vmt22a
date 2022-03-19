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


    using Definition_variant = std::variant<
        ast::definition::Function                const*,
        ast::definition::Struct                  const*,
        ast::definition::Data                    const*,
        ast::definition::Alias                   const*,
        ast::definition::Implementation          const*,
        ast::definition::Instantiation           const*,
        ast::definition::Typeclass               const*,

        ast::definition::Function_template       const*,
        ast::definition::Struct_template         const*,
        ast::definition::Data_template           const*,
        ast::definition::Alias_template          const*,
        ast::definition::Implementation_template const*,
        ast::definition::Instantiation_template  const*,
        ast::definition::Typeclass_template      const*
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
            return std::vformat_to(out, fmt, std::make_format_args(args...));
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
            [&]<bu::one_of<ast::definition::Instantiation, ast::definition::Implementation> T>(T const* impl_or_inst)
            {
                auto fmt = [&](auto const& xs) {
                    for (auto& x : xs.span() | std::views::transform(bu::second)) {
                        std::format_to(context.out(), "\n{}", x);
                    }
                };

                if constexpr (std::same_as<T, ast::definition::Instantiation>) {
                    format(
                        "inst{} {} {} {{",
                        template_parameters(),
                        impl_or_inst->typeclass,
                        impl_or_inst->instance
                    );
                }
                else {
                    format(
                        "impl{} {} {{",
                        template_parameters(),
                        impl_or_inst->type
                    );
                }

                fmt(impl_or_inst->function_definitions);
                fmt(impl_or_inst->function_template_definitions);
                fmt(impl_or_inst->alias_definitions);
                fmt(impl_or_inst->alias_template_definitions);

                return format("\n}}");
            },
            [&](ast::definition::Typeclass const* typeclass) {
                format("class {}{} {{", typeclass->name, template_parameters());

                for (auto& signature : typeclass->function_signatures) {
                    format(
                        "fn {}[{}]({}): {}\n",
                        signature.name,
                        signature.template_parameters,
                        signature.type.argument_types,
                        signature.type.return_type
                    );
                }
                for (auto& signature : typeclass->type_signatures) {
                    format(
                        "alias {}[{}]: {}\n",
                        signature.name,
                        signature.template_parameters,
                        signature.classes
                    );
                }

                return format("}}");
            },
            [&]<class T>(ast::definition::Template_definition<T> const* pointer) {
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

DEFINE_FORMATTER_FOR(ast::definition::Implementation) {
    return format_definition(context, &value);
}
DEFINE_FORMATTER_FOR(ast::definition::Implementation_template) {
    return format_definition(context, &value);
}

DEFINE_FORMATTER_FOR(ast::definition::Instantiation) {
    return format_definition(context, &value);
}
DEFINE_FORMATTER_FOR(ast::definition::Instantiation_template) {
    return format_definition(context, &value);
}

DEFINE_FORMATTER_FOR(ast::definition::Typeclass) {
    return format_definition(context, &value);
}
DEFINE_FORMATTER_FOR(ast::definition::Typeclass_template) {
    return format_definition(context, &value);
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


DEFINE_FORMATTER_FOR(ast::Namespace) {
    auto out = context.out();
    std::format_to(out, "module {} {{", value.name);

    for (auto definition : value.definitions_in_order) {
        std::visit([&](auto* const pointer) {
            std::format_to(out, "\n{}", *pointer);
        }, definition);
    }

    return std::format_to(out, "\n}}\n");
}


DEFINE_FORMATTER_FOR(ast::Module) {
    return std::format_to(
        context.out(),
        "{}\n{}\n{}\n{}\n{}",
        value.global_namespace,
        value.instantiations,
        value.instantiation_templates,
        value.implementations,
        value.implementation_templates
    );
}