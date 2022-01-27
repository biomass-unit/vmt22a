#include "bu/utilities.hpp"
#include "bu/uninitialized.hpp"
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
        else {
            return ast::type::Typename { id };
        }
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

    auto extract_array_or_list(Parse_context& context) -> ast::Type {
        auto element_type = extract_type(context);
        bu::Uninitialized<ast::Type> type;

        if (context.try_consume(Token::Type::semicolon)) {
            if (auto token = context.try_extract(Token::Type::integer)) {
                type.initialize(
                    ast::type::Array {
                        std::move(element_type),
                        static_cast<bu::Usize>(token->as_integer())
                    }
                );
            }
            else {
                throw context.expected("the array length");
            }
        }
        else {
            type.initialize(ast::type::List { std::move(element_type) });
        }

        context.consume_required(Token::Type::bracket_close);
        return std::move(type.read_initialized());
    }

}


auto parser::parse_type(Parse_context& context) -> std::optional<ast::Type> {
    switch (context.extract().type) {
    case Token::Type::upper_name:
        return extract_typename(context);
    case Token::Type::type_of:
        return extract_type_of(context);
    case Token::Type::bracket_open:
        return extract_array_or_list(context);
    default:
        --context.pointer;
        return std::nullopt;
    }
}