#include "bu/utilities.hpp"
#include "parser_internals.hpp"


namespace {

    using namespace parser;

    template <class T>
    auto extract_literal(Parse_context& context) -> std::optional<ast::Expression> {
        static_assert(std::is_trivially_copyable_v<lexer::Token>); // ensure cheap copy
        return ast::Literal<T> { context.extract().value_as<T>() };
    }

    auto parse_literal(Parse_context& context) -> std::optional<ast::Expression> {
        switch (context.pointer->type) {
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
        default:
            return std::nullopt;
        }
    }

    auto parse_tuple(Parse_context& context) -> std::optional<ast::Expression> {
        if (context.try_consume(Token::Type::paren_open)) {
            auto expressions = extract_comma_separated_zero_or_more<parse_expression>(context);
            if (context.try_consume(Token::Type::paren_close)) {
                return ast::Tuple { std::move(expressions) };
            }
            else {
                bu::abort("expected a closing ')'");
            }
        }
        else {
            return std::nullopt;
        }
    }

}


auto parser::parse_expression(Parse_context& context) -> std::optional<ast::Expression> {
    return parse_one_of<
        parse_literal,
        parse_tuple
    >(context);
}