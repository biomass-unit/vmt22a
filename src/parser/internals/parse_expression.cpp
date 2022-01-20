#include "bu/utilities.hpp"
#include "parser_internals.hpp"


namespace {

    using namespace parser;


    template <class T>
    auto extract_literal(Parse_context& context) -> ast::Expression {
        return ast::Literal<T> { context.pointer[-1].value_as<T>() };
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
        constexpr auto extract_expressions =
            extract_separated_zero_or_more<parse_expression, Token::Type::semicolon, "an expression">;

        auto expressions = extract_expressions(context);
        context.consume_required(Token::Type::brace_close);
        return ast::Compound_expression { std::move(expressions) };
    }
    else {
        return std::nullopt;
    }
}