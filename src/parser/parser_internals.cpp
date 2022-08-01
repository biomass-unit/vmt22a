#include "bu/utilities.hpp"
#include "parser_internals.hpp"


auto parser::parse_top_level_pattern(Parse_context& context) -> std::optional<ast::Pattern> {
    return parse_comma_separated_one_or_more<parse_pattern, "a pattern">(context)
        .transform([](std::vector<ast::Pattern>&& patterns) -> ast::Pattern
    {
        if (patterns.size() != 1) {
            auto source_view = patterns.front().source_view + patterns.back().source_view;
            return ast::Pattern {
                .value = ast::pattern::Tuple { .patterns = std::move(patterns) },
                .source_view = std::move(source_view)
            };
        }
        else {
            return std::move(patterns.front());
        }
    });
}


auto parser::parse_template_arguments(Parse_context& context)
    -> std::optional<std::vector<ast::Template_argument>>
{
    static constexpr auto extract_arguments = extract_comma_separated_zero_or_more<[](Parse_context& context)
        -> std::optional<ast::Template_argument>
    {
        if (auto type = parse_type(context)) {
            return ast::Template_argument { bu::wrap(std::move(*type)) };
        }
        else if (auto expression = parse_expression(context)) {
            return ast::Template_argument { bu::wrap(std::move(*expression)) };
        }
        else {
            std::optional<ast::Mutability> mutability;

            auto* const anchor = context.pointer;

            auto const get_source_view = [&] {
                return make_source_view(anchor, context.pointer - 1);
            };

            if (context.try_consume(Token::Type::mut)) {
                if (context.try_consume(Token::Type::question)) {
                    mutability.emplace(
                        ast::Mutability {
                            .parameter_name = extract_lower_id(context, "a mutability parameter name"),
                            .type           = ast::Mutability::Type::parameterized
                        }
                    );
                }
                else {
                    mutability.emplace(ast::Mutability { .type = ast::Mutability::Type::mut });
                }
            }
            else if (context.try_consume(Token::Type::immut)) {
                mutability.emplace(ast::Mutability { .type = ast::Mutability::Type::immut });
            }

            return mutability.transform(bu::make<ast::Template_argument>);
        }
    }, "a template argument">;

    if (context.try_consume(Token::Type::bracket_open)) {
        auto arguments = extract_arguments(context);
        context.consume_required(Token::Type::bracket_close);
        return std::move(arguments);
    }
    else {
        return std::nullopt;
    }
}


auto parser::parse_template_parameters(Parse_context& context)
    -> std::optional<std::vector<ast::Template_parameter>>
{
    constexpr auto extract_parameters = parse_comma_separated_one_or_more<[](Parse_context& context)
        -> std::optional<ast::Template_parameter>
    {
        auto const get_source_view = [&, anchor = context.pointer] {
            return make_source_view(anchor, context.pointer - 1);
        };

        if (auto name = parse_lower_name(context)) {
            if (context.try_consume(Token::Type::colon)) {
                if (context.try_consume(Token::Type::mut)) {
                    return ast::Template_parameter {
                        .value       = ast::Template_parameter::Mutability_parameter {},
                        .name        = std::move(*name),
                        .source_view = get_source_view()
                    };
                }
                else if (auto type = parse_type(context)) {
                    return ast::Template_parameter {
                        .value       = ast::Template_parameter::Value_parameter { std::move(*type) },
                        .name        = std::move(*name),
                        .source_view = get_source_view()
                    };
                }
                else {
                    context.error_expected("'mut' or a type");
                }
            }

            return ast::Template_parameter {
                .value       = ast::Template_parameter::Value_parameter { .type = std::nullopt },
                .name        = std::move(*name),
                .source_view = get_source_view()
            };
        }
        else if (auto name = parse_upper_name(context)) {
            std::vector<ast::Class_reference> classes;
            if (context.try_consume(Token::Type::colon)) {
                classes = extract_class_references(context);
            }

            return ast::Template_parameter {
                .value       = ast::Template_parameter::Type_parameter { std::move(classes) },
                .name        = std::move(*name),
                .source_view = get_source_view()
            };
        }
        else {
            return std::nullopt;
        }
    }, "a template parameter">;

    if (context.try_consume(Token::Type::bracket_open)) {
        if (auto parameters = extract_parameters(context)) {
            context.consume_required(Token::Type::bracket_close);
            return parameters;
        }
        else {
            context.error_expected("one or more template parameters");
        }
    }
    else {
        return std::nullopt;
    }
}


auto parser::extract_function_parameters(Parse_context& context)
    -> std::vector<ast::Function_parameter>
{
    return extract_comma_separated_zero_or_more<
        [](Parse_context& context)
            -> std::optional<ast::Function_parameter>
        {
            if (auto pattern = parse_pattern(context)) {
                std::optional<ast::Type> type;
                if (context.try_consume(Token::Type::colon)) {
                    type = extract_type(context);
                }

                std::optional<ast::Expression> default_value;
                if (context.try_consume(Token::Type::equals)) {
                    default_value = extract_expression(context);
                }

                return ast::Function_parameter {
                    std::move(*pattern),
                    std::move(type),
                    std::move(default_value)
                };
            }
            else {
                return std::nullopt;
            }
        },
        "a function parameter"
    >(context);
}


auto parser::extract_qualified(ast::Root_qualifier&& root, Parse_context& context)
    -> ast::Qualified_name
{
    std::vector<ast::Qualifier> qualifiers;
    lexer::Token* template_argument_anchor = nullptr;

    auto extract_qualifier = [&]() -> bool {
        switch (auto& token = context.extract(); token.type) {
        case Token::Type::lower_name:
        case Token::Type::upper_name:
        {
            template_argument_anchor = context.pointer;
            auto template_arguments = parse_template_arguments(context);

            ast::Qualifier qualifier {
                .template_arguments = std::move(template_arguments),
                .name {
                    .identifier  = token.as_identifier(),
                    .is_upper    = token.type == Token::Type::upper_name,
                    .source_view = template_argument_anchor[-1].source_view
                },
                .source_view = make_source_view(template_argument_anchor - 1, context.pointer - 1)
            };

            qualifiers.push_back(std::move(qualifier));
            return true;
        }
        default:
            context.retreat();
            return false;
        }
    };

    if (extract_qualifier()) {
        while (context.try_consume(Token::Type::double_colon)) {
            if (!extract_qualifier()) {
                context.error_expected("an identifier");
            }
        }

        auto back = std::move(qualifiers.back());
        qualifiers.pop_back();

        // Ignore potential template arguments, they are handled separately
        context.pointer = template_argument_anchor;

        return {
            .middle_qualifiers = std::move(qualifiers),
            .root_qualifier    = std::move(root),
            .primary_name      = std::move(back.name)
        };
    }
    else {
        // root:: followed by no qualifiers
        context.error_expected("an identifier");
    }
}


auto parser::extract_mutability(Parse_context& context) -> ast::Mutability {
    auto* const anchor = context.pointer;

    auto const get_source_view = [&] {
        return context.pointer == anchor
            ? anchor->source_view
            : make_source_view(anchor, context.pointer - 1);
    };

    if (context.try_consume(Token::Type::mut)) {
        if (context.try_consume(Token::Type::question)) {
            return {
                .parameter_name = extract_lower_id(context, "a mutability parameter name"),
                .type           = ast::Mutability::Type::parameterized
            };
        }
        else {
            return { .type = ast::Mutability::Type::mut };
        }
    }
    else {
        return { .type = ast::Mutability::Type::immut };
    }
}


auto parser::parse_class_reference(Parse_context& context)
    -> std::optional<ast::Class_reference>
{
    auto* const anchor = context.pointer;

    auto name = [&]() -> std::optional<ast::Qualified_name> {
        ast::Root_qualifier root;
        auto* const anchor = context.pointer;

        if (context.try_consume(Token::Type::upper_name) || context.try_consume(Token::Type::lower_name)) {
            context.retreat();
        }
        else if (context.try_consume(Token::Type::double_colon)) {
            root.value = ast::Root_qualifier::Global{};
        }
        else {
            return std::nullopt;
        }

        auto name = extract_qualified(std::move(root), context);

        if (name.primary_name.is_upper) {
            return name;
        }
        else {
            context.error(
                { anchor, context.pointer },
                "Expected a class name, but found a lowercase identifier"
            );
        }
    }();

    if (name) {
        auto template_arguments = parse_template_arguments(context);

        return ast::Class_reference {
            .template_arguments = std::move(template_arguments),
            .name               = std::move(*name),
            .source_view        = make_source_view(anchor, context.pointer - 1)
        };
    }
    else {
        return std::nullopt;
    }
}


auto parser::extract_class_references(Parse_context& context)
    -> std::vector<ast::Class_reference>
{
    static constexpr auto extract_classes =
        extract_separated_zero_or_more<parse_class_reference, Token::Type::plus, "a class name">;

    auto classes = extract_classes(context);

    if (classes.empty()) {
        context.error_expected("one or more class names");
    }
    else {
        return classes;
    }
}