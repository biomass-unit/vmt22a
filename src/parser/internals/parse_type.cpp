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

}


auto parser::parse_type(Parse_context& context) -> std::optional<ast::Type> {
    switch (context.extract().type) {
    case Token::Type::upper_name:
        return extract_typename(context);
    default:
        --context.pointer;
        return std::nullopt;
    }
}