#include "bu/utilities.hpp"
#include "parser_internals.hpp"


namespace {

    using namespace parser;


    template <class T>
    auto extract_literal(Parse_context& context) -> ast::Pattern {
        return ast::pattern::Literal<T> { context.previous().value_as<T>() };
    }

    auto extract_tuple(Parse_context& context) -> ast::Pattern {
        auto patterns = extract_comma_separated_zero_or_more<parse_pattern, "a pattern">(context);
        context.consume_required(Token::Type::paren_close);
        return ast::pattern::Tuple { std::move(patterns) };
    }

    auto extract_name(Parse_context& context) -> ast::Pattern {
        context.retreat();
        auto mutability = extract_mutability(context);

        return ast::pattern::Name {
            .identifier = extract_lower_id<"a lowercase identifier">(context),
            .mutability = std::move(mutability)
        };
    }

    auto parse_normal_pattern(Parse_context& context) -> std::optional<ast::Pattern> {
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
        case Token::Type::mut:
            return extract_name(context);
        default:
            context.retreat();
            return std::nullopt;
        }
    }

}


auto parser::parse_pattern(Parse_context& context) -> std::optional<ast::Pattern> {
    return parse_and_add_source_view<[](Parse_context& context) {
        auto pattern = parse_normal_pattern(context);

        if (pattern) {
            if (context.try_consume(Token::Type::if_)) {
                if (auto guard = parse_expression(context)) {
                    *pattern = ast::pattern::Guarded {
                        std::move(*pattern),
                        std::move(*guard)
                    };
                }
                else {
                    throw context.expected("a guard expression");
                }
            }
        }
        
        return pattern;
    }>(context);
}