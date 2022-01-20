#include "bu/utilities.hpp"
#include "parser_internals.hpp"


namespace {

    using namespace parser;


    template <class T>
    auto extract_literal(Parse_context& context) -> ast::Pattern {
        return ast::pattern::Literal<T> { context.pointer[-1].value_as<T>() };
    }

    auto extract_tuple(Parse_context& context) -> ast::Pattern {
        auto patterns = extract_comma_separated_zero_or_more<parse_pattern, "a pattern">(context);
        context.consume_required(Token::Type::paren_close);
        return ast::pattern::Tuple { std::move(patterns) };
    }

}


auto parser::parse_pattern(Parse_context& context) -> std::optional<ast::Pattern> {
    switch (context.extract().type) {
    case Token::Type::underscore:
        return ast::pattern::Wildcard {};
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
    case Token::Type::lower_name:
        return ast::pattern::Name { context.pointer[-1].as_identifier() };
    default:
        --context.pointer;
        return std::nullopt;
    }
}