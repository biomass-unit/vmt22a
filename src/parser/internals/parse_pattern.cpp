#include "bu/utilities.hpp"
#include "parser_internals.hpp"


namespace {

    using namespace parser;

}


auto parser::parse_pattern(Parse_context& context) -> std::optional<ast::Pattern> {
    switch (context.extract().type) {
    case Token::Type::underscore:
        return ast::pattern::Wildcard {};
    default:
        return std::nullopt;
    }
}