#include "bu/utilities.hpp"
#include "parser_internals.hpp"


namespace {

    using namespace parser;


    template <class T>
    auto extract_literal(Parse_context& context) -> ast::Expression {
        return ast::Literal<T> { context.previous().value_as<T>() };
    }

    auto extract_variable(Parse_context& context) -> ast::Expression {
        return ast::Variable { context.previous().as_identifier() };
    }

    auto extract_tuple(Parse_context& context) -> ast::Expression {
        auto expressions = extract_comma_separated_zero_or_more<parse_expression, "an expression">(context);
        context.consume_required(Token::Type::paren_close);
        if (expressions.size() == 1) {
            return std::move(expressions.front());
        }
        else {
            return ast::Tuple { std::move(expressions) };
        }
    }

    auto extract_conditional(Parse_context& context) -> ast::Expression {
        constexpr std::string_view help =
            "the branches of a conditional expression must be compound expressions";

        if (auto condition = parse_expression(context)) {
            if (auto true_branch = parse_compound_expression(context)) {
                std::optional<bu::Wrapper<ast::Expression>> false_branch;

                if (context.try_consume(Token::Type::else_)) {
                    if (auto branch = parse_compound_expression(context)) {
                        false_branch.emplace(std::move(*branch));
                    }
                    else {
                        throw context.expected("the false branch", help);
                    }
                }

                return ast::Conditional {
                    std::move(*condition),
                    std::move(*true_branch),
                    std::move(false_branch)
                };
            }
            else {
                throw context.expected("the true branch", help);
            }
        }
        else {
            throw context.expected("a condition");
        }
    }

    auto extract_let_binding(Parse_context& context) -> ast::Expression {
        auto pattern = extract_pattern(context);
        
        std::optional<bu::Wrapper<ast::Type>> type;
        if (context.try_consume(Token::Type::colon)) {
            type.emplace(extract_type(context));
        }

        context.consume_required(Token::Type::equals);

        return ast::Let_binding {
            std::move(pattern),
            extract_expression(context),
            std::move(type)
        };
    }

    auto extract_loop_body(Parse_context& context) -> ast::Expression {
        if (auto body = parse_compound_expression(context)) {
            return std::move(*body);
        }
        else {
            throw context.expected("the loop body", "the loop body must be a compound expression");
        }
    }

    auto extract_infinite_loop(Parse_context& context) -> ast::Expression {
        return ast::Infinite_loop { extract_loop_body(context) };
    }

    auto extract_while_loop(Parse_context& context) -> ast::Expression {
        auto condition = extract_expression(context);
        return ast::While_loop {
            std::move(condition),
            extract_loop_body(context)
        };
    }

    auto extract_for_loop(Parse_context& context) -> ast::Expression {
        auto iterator = extract_pattern(context);
        context.consume_required(Token::Type::in);
        auto iterable = extract_expression(context);
        return ast::For_loop {
            std::move(iterator),
            std::move(iterable),
            extract_loop_body(context)
        };
    }

    auto extract_size_of(Parse_context& context) -> ast::Expression {
        if (context.try_consume(Token::Type::paren_open)) {
            auto type = extract_type(context);
            context.consume_required(Token::Type::paren_close);
            return ast::Size_of { std::move(type) };
        }
        else {
            throw context.expected("a parenthesized type");
        }
    }

    auto parse_match_case(Parse_context& context) -> std::optional<ast::Match::Case> {
        if (auto pattern = parse_pattern(context)) {
            context.consume_required(Token::Type::right_arrow);
            return ast::Match::Case {
                std::move(*pattern),
                extract_expression(context)
            };
        }
        else {
            return std::nullopt;
        }
    }

    auto extract_match(Parse_context& context) -> ast::Expression {
        constexpr auto parse_cases = braced<
            parse_separated_one_or_more<
                parse_match_case,
                Token::Type::semicolon,
                "a match case"
            >,
            "one or more match cases delimited by ';'"
        >;

        auto expression = extract_expression(context);

        if (auto cases = parse_cases(context)) {
            return ast::Match {
                std::move(*cases),
                std::move(expression)
            };
        }
        else {
            throw context.expected("a '{' followed by match cases");
        }
    }

    auto extract_continue(Parse_context&) -> ast::Expression {
        return ast::Continue {};
    }

    auto extract_break(Parse_context& context) -> ast::Expression {
        return ast::Break { bu::map(parse_expression(context), bu::make_wrapper<ast::Expression>) };
    }

    auto extract_ret(Parse_context& context) -> ast::Expression {
        return ast::Ret { bu::map(parse_expression(context), bu::make_wrapper<ast::Expression>) };
    }

    auto extract_meta(Parse_context& context) -> ast::Expression {
        if (context.try_consume(Token::Type::paren_open)) {
            auto expression = extract_expression(context);
            context.consume_required(Token::Type::paren_close);
            return ast::Meta { std::move(expression) };
        }
        else {
            throw context.expected("a parenthesized expression");
        }
    }

    auto extract_compound_expression(Parse_context& context) -> ast::Expression {
        std::vector<ast::Expression> expressions;

        if (auto head = parse_expression(context)) {
            expressions.push_back(std::move(*head));

            while (context.try_consume(Token::Type::semicolon)) {
                if (auto expression = parse_expression(context)) {
                    expressions.push_back(std::move(*expression));
                }
                else {
                    expressions.push_back(ast::unit_value);
                }
            }
        }

        context.consume_required(Token::Type::brace_close);

        switch (expressions.size()) {
        case 0:
            return ast::unit_value;
        case 1:
            return std::move(expressions.front());
        default:
            return ast::Compound_expression { std::move(expressions) };
        }
    }


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
            return extract_variable(context);
        case Token::Type::paren_open:
            return extract_tuple(context);
        case Token::Type::if_:
            return extract_conditional(context);
        case Token::Type::let:
            return extract_let_binding(context);
        case Token::Type::loop:
            return extract_infinite_loop(context);
        case Token::Type::while_:
            return extract_while_loop(context);
        case Token::Type::for_:
            return extract_for_loop(context);
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
        case Token::Type::meta:
            return extract_meta(context);
        case Token::Type::brace_open:
            return extract_compound_expression(context);
        default:
            --context.pointer;
            return std::nullopt;
        }
    }


    auto extract_arguments(Parse_context& context) -> std::vector<ast::Expression> {
        constexpr auto extract_arguments =
            extract_comma_separated_zero_or_more<parse_expression, "a function argument">;
        auto arguments = extract_arguments(context);

        context.consume_required(Token::Type::paren_close);
        return arguments;
    }


    auto parse_potential_invocation(Parse_context& context) -> std::optional<ast::Expression> {
        if (auto potential_invocable = parse_normal_expression(context)) {
            while (context.try_consume(Token::Type::paren_open)) {
                *potential_invocable = ast::Invocation {
                    extract_arguments(context),
                    std::move(*potential_invocable)
                };
            }
            return potential_invocable;
        }
        else {
            return std::nullopt;
        }
    }


    auto parse_potential_member_access(Parse_context& context) -> std::optional<ast::Expression> {
        auto expression = parse_potential_invocation(context);

        if (expression) {
            while (context.try_consume(Token::Type::dot)) {
                auto member_name = extract_lower_id<"a member name">(context);

                if (context.try_consume(Token::Type::paren_open)) {
                    *expression = ast::Member_function_invocation {
                        extract_arguments(context),
                        std::move(*expression),
                        member_name
                    };
                }
                else {
                    *expression = ast::Member_access {
                        std::move(*expression),
                        member_name
                    };
                }
            }
        }

        return expression;
    }


    auto parse_potential_type_cast(Parse_context& context) -> std::optional<ast::Expression> {
        if (auto expression = parse_potential_member_access(context)) {
            while (context.try_consume(Token::Type::as)) {
                *expression = ast::Type_cast {
                    std::move(*expression),
                    extract_type(context)
                };
            }
            return expression;
        }
        else {
            return std::nullopt;
        }
    }


    template <bu::Metastring... strings>
    std::array id_array {
        lexer::Identifier { strings.view(), lexer::Identifier::guaranteed_new_string }...
    };

    std::tuple precedence_table {
        id_array<"*", "/", "%">,
        id_array<"+", "-">,
        id_array<"<<", ">>">,
        id_array<"?=", "!=">,
        id_array<"<", "<=", ">=", ">">,
        id_array<"&&", "||">,
        id_array<":=", "+=", "*=", "/=", "%=", "<<=", ">>=">
    };

    constexpr auto lowest_precedence = std::tuple_size_v<decltype(precedence_table)> - 1;


    auto parse_operator(Parse_context& context) -> std::optional<lexer::Identifier> {
        if (auto token = context.try_extract(Token::Type::operator_name)) {
            return token->as_identifier();
        }
        else {
            return std::nullopt;
        }
    }

    template <bu::Usize precedence>
    auto parse_binary_operator_invocation_with_precedence(Parse_context& context)
        -> std::optional<ast::Expression>
    {
        constexpr auto recurse =
            parse_binary_operator_invocation_with_precedence<precedence - 1>;

        if (auto left = recurse(context)) {
            while (auto op = parse_operator(context)) {
                if constexpr (precedence != lowest_precedence) {
                    constexpr auto& ops = std::get<precedence>(precedence_table);
                    if (std::ranges::find(ops, op) == ops.end()) {
                        --context.pointer; // not in current operator group
                        break;
                    }
                }
                if (auto right = recurse(context)) {
                    *left = ast::Binary_operator_invocation {
                        std::move(*left),
                        std::move(*right),
                        *op
                    };
                }
                else {
                    throw context.expected("an operand");
                }
            }
            return left;
        }
        else {
            return std::nullopt;
        }
    }

    template <>
    auto parse_binary_operator_invocation_with_precedence<std::numeric_limits<bu::Usize>::max()>(Parse_context& context)
        -> std::optional<ast::Expression>
    {
        return parse_potential_type_cast(context);
    }

}


auto parser::parse_expression(Parse_context& context) -> std::optional<ast::Expression> {
    return parse_and_add_source_view<
        parse_binary_operator_invocation_with_precedence<lowest_precedence>
    >(context);
}

auto parser::parse_compound_expression(Parse_context& context) -> std::optional<ast::Expression> {
    return parse_and_add_source_view<
        [](Parse_context& context) -> std::optional<ast::Expression> {
            if (context.try_consume(Token::Type::brace_open)) {
                return extract_compound_expression(context);
            }
            else {
                return std::nullopt;
            }
        }
    >(context);
}