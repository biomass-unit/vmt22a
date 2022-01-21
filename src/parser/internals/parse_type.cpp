#include "bu/utilities.hpp"
#include "parser_internals.hpp"


namespace {

    using namespace parser;


    auto extract_typename(Parse_context& context) -> ast::Type {
        constexpr std::hash<std::string_view> hasher;

        static auto const
            integer   = hasher("Int"),
            floating  = hasher("Float"),
            character = hasher("Char"),
            boolean   = hasher("Bool"),
            string    = hasher("String");

        auto const id = context.previous().as_identifier();
        auto const hash = id.hash();

        if (hash == integer) {
            return ast::type::Int {};
        }
        else if (hash == floating) {
            return ast::type::Float {};
        }
        else if (hash == character) {
            return ast::type::Char {};
        }
        else if (hash == boolean) {
            return ast::type::Bool {};
        }
        else if (hash == string) {
            return ast::type::String {};
        }

        bu::unimplemented();
    }

    auto extract_type_of(Parse_context& context) -> ast::Type {
        if (context.try_consume(Token::Type::paren_open)) {
            auto type = extract_expression(context);
            context.consume_required(Token::Type::paren_close);
            return ast::type::Type_of { std::move(type) };
        }
        else {
            throw context.expected("a parenthesized expression");
        }
    }

}


auto parser::parse_type(Parse_context& context) -> std::optional<ast::Type> {
    switch (context.extract().type) {
    case Token::Type::upper_name:
        return extract_typename(context);
    case Token::Type::type_of:
        return extract_type_of(context);
    default:
        --context.pointer;
        return std::nullopt;
    }
}