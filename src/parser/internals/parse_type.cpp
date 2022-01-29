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
            return ast::type::integer;
        }
        else if (hash == floating) {
            return ast::type::floating;
        }
        else if (hash == character) {
            return ast::type::character;
        }
        else if (hash == boolean) {
            return ast::type::boolean;
        }
        else if (hash == string) {
            return ast::type::string;
        }
        else {
            return ast::type::Typename { id };
        }
    }

    auto extract_tuple(Parse_context& context) -> ast::Type {
        auto types = extract_comma_separated_zero_or_more<parse_type, "a type">(context);
        context.consume_required(Token::Type::paren_close);
        return ast::type::Tuple { std::move(types) };
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
        return std::move(*type);
    }

    auto extract_function(Parse_context& context) -> ast::Type {
        if (context.try_consume(Token::Type::paren_open)) {
            auto argument_types = extract_comma_separated_zero_or_more<parse_type, "a type">(context);
            context.consume_required(Token::Type::paren_close);
            if (context.try_consume(Token::Type::colon)) {
                if (auto return_type = parse_type(context)) {
                    return ast::type::Function {
                        std::move(argument_types),
                        std::move(*return_type)
                    };
                }
                else {
                    throw context.expected("the function return type");
                }
            }
            else {
                throw context.expected("a ':' followed by the function return type");
            }
        }
        else {
            throw context.expected("a parenthesized list of argument types");
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

}


auto parser::parse_type(Parse_context& context) -> std::optional<ast::Type> {
    switch (context.extract().type) {
    case Token::Type::upper_name:
        return extract_typename(context);
    case Token::Type::paren_open:
        return extract_tuple(context);
    case Token::Type::bracket_open:
        return extract_array_or_list(context);
    case Token::Type::fn:
        return extract_function(context);
    case Token::Type::type_of:
        return extract_type_of(context);
    default:
        --context.pointer;
        return std::nullopt;
    }
}