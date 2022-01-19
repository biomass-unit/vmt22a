#include "bu/utilities.hpp"
#include "parser_internals.hpp"


namespace {

    using namespace parser;


    template <class T>
    auto extract_literal(Parse_context& context) -> ast::Expression {
        static_assert(std::is_trivially_copyable_v<lexer::Token>); // ensure cheap copy
        return ast::Literal<T> { context.pointer[-1].value_as<T>() };
    }

    auto extract_tuple(Parse_context& context) -> ast::Expression {
        auto expressions = extract_comma_separated_zero_or_more<parse_expression, "an expression">(context);
        if (context.try_consume(Token::Type::paren_close)) {
            return ast::Tuple { std::move(expressions) };
        }
        else {
            throw context.expected("a closing ')'");
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
        case Token::Type::paren_open:
            return extract_tuple(context);
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