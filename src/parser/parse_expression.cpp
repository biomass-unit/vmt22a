#include "bu/utilities.hpp"
#include "parser_internals.hpp"


namespace {

    using namespace parser;


    template <class T>
    constexpr Extractor extract_literal = +[](Parse_context& context)
        -> ast::Expression::Variant
    {
        return ast::expression::Literal<T> { context.previous().value_as<T>() };
    };

    template <>
    constexpr Extractor extract_literal<char> = +[](Parse_context& context)
        -> ast::Expression::Variant
    {
        return ast::expression::Literal { static_cast<bu::Char>(context.previous().as_character()) };
    };


    auto parse_struct_member_initializer(Parse_context& context)
        -> std::optional<bu::Pair<lexer::Identifier, ast::Expression>>
    {
        if (auto member = parse_lower_id(context)) {
            context.consume_required(Token::Type::equals);
            return bu::Pair { *member, extract_expression(context) };
        }
        else {
            return std::nullopt;
        }
    };

    auto extract_struct_initializer(ast::Type&& type, Parse_context& context)
        -> ast::Expression::Variant
    {
        static constexpr auto extract_member_initializers =
            extract_comma_separated_zero_or_more<parse_struct_member_initializer, "a member initializer">;

        auto initializers = extract_member_initializers(context);
        context.consume_required(Token::Type::brace_close);

        return ast::expression::Struct_initializer {
            .member_initializers { std::move(initializers) },
            .type                = std::move(type)
        };
    }


    auto extract_qualified_lower_name_or_struct_initializer(ast::Root_qualifier&& root, Parse_context& context)
        -> ast::Expression::Variant
    {
        auto* const anchor = context.pointer;
        auto        name   = extract_qualified(std::move(root), context);

        auto template_arguments = parse_template_arguments(context);

        if (!name.primary_name.is_upper) {
            if (template_arguments) {
                return ast::expression::Template_application {
                    std::move(*template_arguments),
                    std::move(name)
                };
            }
            else {
                return ast::expression::Variable { std::move(name) };
            }
        }
        else if (context.try_consume(Token::Type::brace_open)) {
            auto value = [&]() -> ast::Type::Variant {
                if (template_arguments) {
                    return ast::type::Template_application {
                        std::move(*template_arguments),
                        std::move(name)
                    };
                }
                else {
                    return ast::type::Typename { std::move(name) };
                }
            }();

            return extract_struct_initializer(
                ast::Type {
                    .value       = std::move(value),
                    .source_view = make_source_view(anchor, context.pointer - 1)
                },
                context
            );
        }
        else {
            context.error(
                { anchor, context.pointer },
                "Expected an expression, but found a type"
            );
        }
    }


    constexpr Extractor extract_condition = +[](Parse_context& context)
        -> ast::Expression::Variant
    {
        if (context.try_consume(Token::Type::let)) {
            auto pattern = extract_pattern(context);

            context.consume_required(Token::Type::equals);

            return ast::expression::Conditional_let {
                .pattern     = std::move(pattern),
                .initializer = extract_expression(context)
            };
        }
        else {
            return std::move(extract_expression(context).value);
        }
    };


    auto extract_loop_body(Parse_context& context) -> ast::Expression {
        if (auto body = parse_block_expression(context)) {
            return std::move(*body);
        }
        else {
            context.error_expected("the loop body", "the loop body must be a block expression");
        }
    }

    auto extract_any_loop(Parse_context& context, std::optional<ast::Name> const label = std::nullopt)
        -> ast::Expression::Variant
    {
        context.retreat();

        switch (context.extract().type) {
        case Token::Type::loop:
        {
            return ast::expression::Infinite_loop {
                .label = label,
                .body  = extract_loop_body(context)
            };
        }
        case Token::Type::while_:
        {
            auto condition = extract_condition(context);

            if (auto* const literal = std::get_if<ast::expression::Literal<bool>>(&condition.value)) {
                if (literal->value) {
                    context.diagnostics.emit_simple_note({
                        .erroneous_view = condition.source_view,
                        .source         = context.source,
                        .message        = "Consider using 'loop' instead of 'while true'",
                    });
                }
                else {
                    context.diagnostics.emit_simple_warning({
                        .erroneous_view = condition.source_view,
                        .source         = context.source,
                        .message        = "Loop will never be run"
                    });
                }
            }

            return ast::expression::While_loop {
                .label     = label,
                .condition = std::move(condition),
                .body      = extract_loop_body(context)
            };
        }
        case Token::Type::for_:
        {
            auto iterator = extract_pattern(context);
            context.consume_required(Token::Type::in);
            auto iterable = extract_expression(context);

            return ast::expression::For_loop {
                .label    = label,
                .iterator = std::move(iterator),
                .iterable = std::move(iterable),
                .body     = extract_loop_body(context)
            };
        }
        default:
            bu::abort(); // Should be unreachable
        }
    }

    constexpr Extractor extract_loop = +[](Parse_context& context) -> ast::Expression::Variant {
        return extract_any_loop(context);
    };


    constexpr Extractor extract_identifier = +[](Parse_context& context)
        -> ast::Expression::Variant
    {
        switch (context.pointer->type) {
        case Token::Type::loop:
        case Token::Type::while_:
        case Token::Type::for_:
            if (Token* const name_token = &context.previous(); name_token->type == Token::Type::lower_name) {
                ++context.pointer;
                return extract_any_loop(
                    context,
                    ast::Name {
                        .identifier  = name_token->as_identifier(),
                        .is_upper    = name_token->type == Token::Type::upper_name,
                        .source_view = name_token->source_view
                    }
                );
            }
            else {
                context.error(name_token->source_view, { "Loop labels must be lowercase" });
            }
        default:
            context.retreat();
            return extract_qualified_lower_name_or_struct_initializer({ std::monostate {} }, context);
        }
    };

    constexpr Extractor extract_global_identifier = +[](Parse_context& context)
        -> ast::Expression::Variant
    {
        return extract_qualified_lower_name_or_struct_initializer({ ast::Root_qualifier::Global {} }, context);
    };

    constexpr Extractor extract_dereference = +[](Parse_context& context)
        -> ast::Expression::Variant
    {
        return ast::expression::Dereference { extract_expression(context) };
    };

    constexpr Extractor extract_tuple = +[](Parse_context& context)
        -> ast::Expression::Variant
    {
        auto expressions = extract_comma_separated_zero_or_more<parse_expression, "an expression">(context);
        context.consume_required(Token::Type::paren_close);
        if (expressions.size() == 1) {
            return std::move(expressions.front().value);
        }
        else {
            return ast::expression::Tuple { std::move(expressions) };
        }
    };

    constexpr Extractor extract_array = +[](Parse_context& context)
        -> ast::Expression::Variant
    {
        static constexpr auto extract_elements =
            extract_comma_separated_zero_or_more<parse_expression, "an array element">;

        auto elements = extract_elements(context);

        if (context.try_consume(Token::Type::bracket_close)) {
            return ast::expression::Array_literal { std::move(elements) };
        }
        else if (elements.empty()) {
            context.error_expected("an array element or a ']'");
        }
        else {
            context.error_expected("a ',' or a ']'");
        }
    };

    constexpr Extractor<ast::Expression> extract_conditional = +[](Parse_context& context)
        -> ast::Expression::Variant
    {
        static constexpr std::string_view help =
            "the branches of a conditional expression must be block expressions";

        auto condition = extract_condition(context);

        if (auto true_branch = parse_block_expression(context)) {
            std::optional<bu::Wrapper<ast::Expression>> false_branch;

            Token const* const else_token = context.pointer;

            if (context.try_consume(Token::Type::else_)) {
                if (auto branch = parse_block_expression(context)) {
                    false_branch = std::move(*branch);
                }
                else {
                    context.error_expected("the false branch", help);
                }
            }
            else if (context.try_consume(Token::Type::elif)) {
                false_branch = bu::wrap(extract_conditional(context));
            }

            if (auto* const literal = std::get_if<ast::expression::Literal<bool>>(&condition.value)) {
                static constexpr auto selected_if = [](bool const x) noexcept {
                    return x ? "This branch will always be selected"
                             : "This branch will never be selected";
                };

                std::vector<bu::diagnostics::Text_section> sections;
                sections.push_back({
                    .source_view = condition.source_view,
                    .source      = context.source,
                    .note        = selected_if(literal->value)
                });

                if (false_branch.has_value()) {
                    sections.push_back({
                        .source_view = else_token->source_view,
                        .source      = context.source,
                        .note        = selected_if(!literal->value)
                    });
                }

                context.diagnostics.emit_warning({
                    .sections = std::move(sections),
                    .message  = "Boolean literal condition"
                });
            }

            return ast::expression::Conditional {
                std::move(condition),
                std::move(*true_branch),
                std::move(false_branch)
            };
        }
        else {
            context.error_expected("the true branch", help);
        }
    };

    constexpr Extractor extract_let_binding = +[](Parse_context& context)
        -> ast::Expression::Variant
    {
        auto pattern = extract_required<parse_top_level_pattern, "a pattern">(context);
        
        std::optional<bu::Wrapper<ast::Type>> type;
        if (context.try_consume(Token::Type::colon)) {
            type.emplace(extract_type(context));
        }

        context.consume_required(Token::Type::equals);

        return ast::expression::Let_binding {
            std::move(pattern),
            extract_expression(context),
            std::move(type)
        };
    };

    constexpr Extractor extract_local_type_alias = +[](Parse_context& context)
        -> ast::Expression::Variant
    {
        auto name = extract_upper_id(context, "an alias name");
        context.consume_required(Token::Type::equals);
        return ast::expression::Local_type_alias {
            std::move(name),
            extract_type(context)
        };
    };


    constexpr Extractor extract_lambda = +[](Parse_context& context)
        -> ast::Expression::Variant
    {
        auto parameters = extract_function_parameters(context);

        using Capture = ast::expression::Lambda::Capture;

        static constexpr auto parse_capture = +[](Parse_context& context)
            -> std::optional<Capture>
        {
            auto* const anchor = context.pointer;

            return [&]() -> std::optional<Capture::Variant> {
                if (context.try_consume(Token::Type::ampersand)) {
                    return Capture::By_reference {
                        .variable = extract_lower_id(context, "a variable name")
                    };
                }
                else if (auto pattern = parse_pattern(context)) {
                    context.consume_required(Token::Type::equals);
                    return Capture::By_pattern {
                        .pattern    = std::move(*pattern),
                        .expression = extract_expression(context)
                    };
                }
                else {
                    return std::nullopt;
                }
            }()
                .transform([&](Capture::Variant&& value)
            {
                return Capture {
                    .value       = std::move(value),
                    .source_view = make_source_view(anchor, context.pointer - 1)
                };
            });
        };

        static constexpr auto extract_captures =
            extract_comma_separated_zero_or_more<parse_capture, "a lambda capture">;

        std::vector<ast::expression::Lambda::Capture> captures;
        if (context.try_consume(Token::Type::dot)) {
            captures = extract_captures(context);

            if (captures.empty()) {
                context.error_expected(
                    "at least one lambda capture",
                    "If the lambda isn't supposed to capture anything, "
                    "or if it only captures by move, remove the '.'"
                );
            }
        }

        context.consume_required(Token::Type::right_arrow);
        auto body = extract_expression(context);

        return ast::expression::Lambda {
            .body              = std::move(body),
            .parameters        = std::move(parameters),
            .explicit_captures = std::move(captures)
        };
    };


    constexpr Extractor extract_hole = +[](Parse_context&)
        -> ast::Expression::Variant
    {
        return ast::expression::Hole {};
    };


    constexpr Extractor extract_size_of = +[](Parse_context& context)
        -> ast::Expression::Variant
    {
        if (context.try_consume(Token::Type::paren_open)) {
            auto type = extract_type(context);
            context.consume_required(Token::Type::paren_close);
            return ast::expression::Size_of { std::move(type) };
        }
        else {
            context.error_expected("a parenthesized type");
        }
    };


    auto parse_match_case(Parse_context& context)
        -> std::optional<ast::expression::Match::Case>
    {
        if (auto pattern = parse_top_level_pattern(context)) {
            context.consume_required(Token::Type::right_arrow);
            return ast::expression::Match::Case {
                std::move(*pattern),
                extract_expression(context)
            };
        }
        else {
            return std::nullopt;
        }
    }


    constexpr Extractor extract_match = +[](Parse_context& context)
        -> ast::Expression::Variant
    {
        auto expression = extract_expression(context);

        context.consume_required(Token::Type::brace_open);

        std::vector<ast::expression::Match::Case> cases;
        while (auto match_case = parse_match_case(context)) {
            cases.push_back(std::move(*match_case));
        }

        if (cases.empty()) {
            context.error_expected("one or more match cases");
        }

        context.consume_required(Token::Type::brace_close);

        return ast::expression::Match {
            .cases      = std::move(cases),
            .expression = std::move(expression)
        };
    };

    constexpr Extractor extract_continue = +[](Parse_context&)
        -> ast::Expression::Variant
    {
        return ast::expression::Continue {};
    };

    constexpr Extractor extract_break = +[](Parse_context& context)
        -> ast::Expression::Variant
    {
        std::optional<ast::Name> label;

        {
            Token* const anchor = context.pointer;
            if (auto name = parse_lower_name(context); name && context.try_consume(Token::Type::loop)) {
                label = *name;
            }
            else {
                context.pointer = anchor;
            }
        }

        return ast::expression::Break {
            .label = std::move(label),
            .expression = parse_expression(context).transform(bu::wrap)
        };
    };

    constexpr Extractor extract_ret = +[](Parse_context& context)
        -> ast::Expression::Variant
    {
        return ast::expression::Ret {
            parse_expression(context).transform(bu::make<bu::Wrapper<ast::Expression>>)
        };
    };

    constexpr Extractor extract_take_reference = +[](Parse_context& context)
        -> ast::Expression::Variant
    {
        auto mutability = extract_mutability(context);
        return ast::expression::Take_reference {
            .mutability = std::move(mutability),
            .name       = extract_lower_id(context, "a variable name")
        };
    };

    constexpr Extractor extract_meta = +[](Parse_context& context)
        -> ast::Expression::Variant
    {
        if (context.try_consume(Token::Type::paren_open)) {
            auto expression = extract_expression(context);
            context.consume_required(Token::Type::paren_close);
            return ast::expression::Meta { std::move(expression) };
        }
        else {
            context.error_expected("a parenthesized expression");
        }
    };

    constexpr Extractor extract_block_expression = +[](Parse_context& context)
        -> ast::Expression::Variant
    {
        std::vector<ast::Expression> expressions;

        if (auto head = parse_expression(context)) {
            expressions.push_back(std::move(*head));

            while (context.try_consume(Token::Type::semicolon)) {
                if (auto expression = parse_expression(context)) {
                    expressions.push_back(std::move(*expression));
                }
                else {
                    context.consume_required(Token::Type::brace_close);
                    return ast::expression::Block { .side_effects = std::move(expressions) };
                }
            }
        }

        context.consume_required(Token::Type::brace_close);

        std::optional<bu::Wrapper<ast::Expression>> result;

        if (!expressions.empty()) {
            result = std::move(expressions.back());
            expressions.pop_back();
        }

        return ast::expression::Block {
            .side_effects = std::move(expressions),
            .result = std::move(result)
        };
    };


    auto parse_normal_expression(Parse_context& context) -> std::optional<ast::Expression> {
        switch (context.extract().type) {
        case Token::Type::integer:
            return extract_literal<bu::Isize>(context);
        case Token::Type::floating:
            return extract_literal<bu::Float>(context);
        case Token::Type::character:
            return extract_literal<char>(context);
        case Token::Type::boolean:
            return extract_literal<bool>(context);
        case Token::Type::string:
            return extract_literal<lexer::String>(context);
        case Token::Type::lower_name:
        case Token::Type::upper_name:
            return extract_identifier(context);
        case Token::Type::double_colon:
            return extract_global_identifier(context);
        case Token::Type::asterisk:
            return extract_dereference(context);
        case Token::Type::paren_open:
            return extract_tuple(context);
        case Token::Type::bracket_open:
            return extract_array(context);
        case Token::Type::if_:
            return extract_conditional(context);
        case Token::Type::let:
            return extract_let_binding(context);
        case Token::Type::alias:
            return extract_local_type_alias(context);
        case Token::Type::lambda:
            return extract_lambda(context);
        case Token::Type::hole:
            return extract_hole(context);
        case Token::Type::loop:
        case Token::Type::while_:
        case Token::Type::for_:
            return extract_loop(context);
        case Token::Type::size_of:
            return extract_size_of(context);
        case Token::Type::match:
            return extract_match(context);
        case Token::Type::continue_:
            return extract_continue(context);
        case Token::Type::break_:
            return extract_break(context);
        case Token::Type::ret:
            return extract_ret(context);
        case Token::Type::ampersand:
            return extract_take_reference(context);
        case Token::Type::meta:
            return extract_meta(context);
        case Token::Type::brace_open:
            return extract_block_expression(context);
        default:
        {
            context.retreat();
            auto* const anchor = context.pointer;

            if (auto type = parse_type(context)) {
                auto value = [&] {
                    if (context.try_consume(Token::Type::double_colon)) {
                        return extract_qualified_lower_name_or_struct_initializer(ast::Root_qualifier { std::move(*type) }, context);
                    }
                    else if (context.try_consume(Token::Type::brace_open)) {
                        return extract_struct_initializer(std::move(*type), context);
                    }
                    else {
                        context.error(
                            { anchor, context.pointer },
                            "Expected an expression, but found a type"
                        );
                    }
                }();

                return ast::Expression {
                    .value       = std::move(value),
                    .source_view = make_source_view(anchor, context.pointer - 1)
                };
            }
            else {
                return std::nullopt;
            }
        }
        }
    }


    auto parse_argument(Parse_context& context) -> std::optional<ast::Function_argument> {
        if (auto name = parse_lower_name(context)) {
            if (context.try_consume(Token::Type::equals)) {
                return ast::Function_argument {
                    .expression = extract_expression(context),
                    .name       = std::move(*name)
                };
            }
            else {
                context.retreat();
            }
        }
        return parse_expression(context).transform(bu::make<ast::Function_argument>);
    }

    auto extract_arguments(Parse_context& context) -> std::vector<ast::Function_argument> {
        constexpr auto extract_arguments =
            extract_comma_separated_zero_or_more<parse_argument, "a function argument">;
        auto arguments = extract_arguments(context);

        context.consume_required(Token::Type::paren_close);
        return arguments;
    }


    auto parse_potential_invocation(Parse_context& context)
        -> std::optional<ast::Expression>
    {
        auto* const anchor              = context.pointer;
        auto        potential_invocable = parse_normal_expression(context);

        if (potential_invocable) {
            while (context.try_consume(Token::Type::paren_open)) {
                auto arguments = extract_arguments(context);

                *potential_invocable = ast::Expression {
                    .value = ast::expression::Invocation {
                        std::move(arguments),
                        std::move(*potential_invocable)
                    },
                    .source_view = make_source_view(anchor, context.pointer - 1)
                };
            }
        }

        return potential_invocable;
    }


    auto parse_potential_member_access(Parse_context& context)
        -> std::optional<ast::Expression>
    {
        auto* const anchor     = context.pointer;
        auto        expression = parse_potential_invocation(context);

        if (expression) {
            std::vector<ast::expression::Member_access_chain::Accessor> accessors;

            while (context.try_consume(Token::Type::dot)) {
                using Chain = ast::expression::Member_access_chain;

                if (auto member_name = parse_lower_name(context)) {
                    if (context.try_consume(Token::Type::paren_open)) {
                        if (!accessors.empty()) {
                            *expression = ast::Expression {
                                .value = ast::expression::Member_access_chain {
                                    std::move(accessors),
                                    std::move(*expression)
                                },
                                .source_view = make_source_view(anchor, context.pointer - 1)
                            };
                            accessors = {}; // Silence warnings, even though a moved-from vector is guaranteed to be empty
                        }

                        auto arguments = extract_arguments(context);

                        *expression = ast::Expression {
                            .value = ast::expression::Member_function_invocation {
                                .arguments   = std::move(arguments),
                                .expression  = std::move(*expression),
                                .member_name = *member_name
                            },
                            .source_view = make_source_view(anchor, context.pointer - 1)
                        };
                    }
                    else {
                        accessors.push_back(Chain::Struct_field { *member_name });
                    }
                }
                else if (Token* const member_index = context.try_extract(Token::Type::integer)) {
                    accessors.push_back(Chain::Tuple_field { member_index->as_integer() });
                }
                else if (context.try_consume(Token::Type::bracket_open)) {
                    auto expression = extract_expression(context);
                    context.consume_required(Token::Type::bracket_close);
                    accessors.push_back(Chain::Array_index { std::move(expression) });
                }
                else {
                    context.error_expected("a struct member name (a.b), a tuple member index (a.0), or an array index (a.[b])");
                }
            }

            if (!accessors.empty()) {
                *expression = ast::Expression {
                    .value = ast::expression::Member_access_chain {
                        std::move(accessors),
                        std::move(*expression)
                    },
                    .source_view = make_source_view(anchor, context.pointer - 1)
                };
            }
        }

        return expression;
    }


    auto parse_potential_type_cast(Parse_context& context)
        -> std::optional<ast::Expression>
    {
        auto* const anchor = context.pointer;

        if (auto expression = parse_potential_member_access(context)) {
            bool continue_looping = true;

            while (continue_looping) {
                using Kind = ast::expression::Type_cast::Kind;

                Kind cast_kind = Kind::conversion;

                switch (context.extract().type) {
                case Token::Type::colon:
                    cast_kind = Kind::ascription;
                    [[fallthrough]];
                case Token::Type::as:
                {
                    auto type = extract_type(context);
                    *expression = ast::Expression {
                        .value = ast::expression::Type_cast {
                            .expression = std::move(*expression),
                            .target     = std::move(type),
                            .kind       = cast_kind
                        },
                        .source_view = make_source_view(anchor, context.pointer - 1)
                    };
                    break;
                }
                default:
                    context.retreat();
                    continue_looping = false;
                }
            }

            return expression;
        }
        else {
            return std::nullopt;
        }
    }


    template <bu::Metastring... strings>
    std::array const id_array {
        lexer::Identifier { strings.view(), bu::Pooled_string_strategy::guaranteed_new_string }...
    };

    std::tuple const precedence_table {
        id_array<"*", "/", "%">,
        id_array<"+", "-">,
        id_array<"?=", "!=">,
        id_array<"<", "<=", ">=", ">">,
        id_array<"&&", "||">,
        id_array<":=", "+=", "*=", "/=", "%=">
    };

    constexpr bu::Usize lowest_precedence = std::tuple_size_v<decltype(precedence_table)> - 1;


    auto parse_operator(Parse_context& context) -> std::optional<lexer::Identifier> {
        static std::optional const asterisk = [] {
            auto const identifier = std::get<0>(precedence_table).front();
            assert(identifier.view() == "*");
            return identifier;
        }();
        static std::optional const plus = [] {
            auto const identifier = std::get<1>(precedence_table).front();
            assert(identifier.view() == "+");
            return identifier;
        }();

        switch (context.extract().type) {
        case Token::Type::operator_name:
            return context.previous().as_identifier();
        case Token::Type::asterisk:
            return asterisk;
        case Token::Type::plus:
            return plus;
        default:
            context.retreat();
            return std::nullopt;
        }
    }

    template <bu::Usize precedence>
    auto parse_binary_operator_invocation_with_precedence(Parse_context& context)
        -> std::optional<ast::Expression>
    {
        static constexpr auto recurse =
            parse_binary_operator_invocation_with_precedence<precedence - 1>;

        auto* const anchor = context.pointer;

        if (auto left = recurse(context)) {
            while (auto op = parse_operator(context)) {
                if constexpr (precedence != lowest_precedence) {
                    constexpr auto& ops = std::get<precedence>(precedence_table);
                    if (std::ranges::find(ops, op) == ops.end()) {
                        context.retreat(); // not in current operator group
                        break;
                    }
                }
                if (auto right = recurse(context)) {
                    *left = ast::Expression {
                        .value = ast::expression::Binary_operator_invocation {
                            std::move(*left),
                            std::move(*right),
                            *op
                        },
                        .source_view = make_source_view(anchor, context.pointer - 1)
                    };
                }
                else {
                    context.error_expected("an operand");
                }
            }
            return left;
        }
        else {
            return std::nullopt;
        }
    }

    template <>
    auto parse_binary_operator_invocation_with_precedence<static_cast<bu::Usize>(-1)>(Parse_context& context)
        -> std::optional<ast::Expression>
    {
        return parse_potential_type_cast(context);
    }

}


auto parser::parse_expression(Parse_context& context) -> std::optional<ast::Expression> {
    return parse_binary_operator_invocation_with_precedence<lowest_precedence>(context);
}

auto parser::parse_block_expression(Parse_context& context) -> std::optional<ast::Expression> {
    if (context.try_consume(Token::Type::brace_open)) {
        return extract_block_expression(context);
    }
    else {
        return std::nullopt;
    }
}