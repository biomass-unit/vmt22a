#include "bu/utilities.hpp"
#include "parser_internals.hpp"


namespace {

    using namespace parser;


    template <class T>
    auto extract_literal(Parse_context& context) -> ast::Expression {
        return ast::Literal<T> { context.previous().value_as<T>() };
    }

    auto extract_tuple(Parse_context& context) -> ast::Expression {
        auto expressions = extract_comma_separated_zero_or_more<parse_expression, "an expression">(context);
        context.consume_required(Token::Type::paren_close);
        return ast::Tuple { std::move(expressions) };
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
        constexpr auto extract_expressions =
            extract_separated_zero_or_more<parse_expression, Token::Type::semicolon, "an expression">;

        auto expressions = extract_expressions(context);
        context.consume_required(Token::Type::brace_close);
        return ast::Compound_expression { std::move(expressions) };
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

}


auto parser::parse_expression(Parse_context& context) -> std::optional<ast::Expression> {
    return parse_normal_expression(context);
}

auto parser::parse_compound_expression(Parse_context& context) -> std::optional<ast::Expression> {
    if (context.try_consume(Token::Type::brace_open)) {
        return extract_compound_expression(context);
    }
    else {
        return std::nullopt;
    }
}