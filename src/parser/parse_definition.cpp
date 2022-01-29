#include "bu/utilities.hpp"
#include "bu/uninitialized.hpp"
#include "internals/parser_internals.hpp"


namespace {

    using namespace parser;


    auto parse_function_parameter(Parse_context& context)
        -> std::optional<ast::definition::Function::Parameter>
    {
        if (auto pattern = parse_pattern(context)) {
            context.consume_required(Token::Type::colon);
            return ast::definition::Function::Parameter {
                std::move(*pattern),
                extract_type(context)
            };
        }
        else {
            return std::nullopt;
        }
    }

    auto extract_function(Parse_context& context, ast::Module& module) -> void {
        if (auto name = parse_lower_id(context)) {
            if (context.try_consume(Token::Type::paren_open)) {
                constexpr auto parse_parameters =
                    extract_comma_separated_zero_or_more<parse_function_parameter, "a function parameter">;
                auto parameters = parse_parameters(context);
                context.consume_required(Token::Type::paren_close);

                std::optional<bu::Wrapper<ast::Type>> return_type;
                if (context.try_consume(Token::Type::colon)) {
                    return_type.emplace(extract_type(context));
                }

                bu::Uninitialized<ast::Expression> body;

                if (auto expression = parse_compound_expression(context)) {
                    body.initialize(std::move(*expression));
                }
                else if (context.try_consume(Token::Type::equals)) {
                    body.initialize(extract_expression(context));
                }
                else {
                    throw context.expected("the function body", "'=' or '{'");
                }

                module.global_namespace->function_definitions.push_back(
                    ast::definition::Function {
                        .body        = std::move(body.read_initialized()),
                        .parameters  = std::move(parameters),
                        .name        = *name,
                        .return_type = return_type
                    }
                );
            }
            else {
                throw context.expected("a parenthesized list of function parameters");
            }
        }
        else {
            throw context.expected("a function name");
        }
    }

}


auto parser::parse_definition(Parse_context& context, ast::Module& module) -> bool {
    switch (context.extract().type) {
    case Token::Type::fn:
        extract_function(context, module);
        return true;
    default:
        --context.pointer;
        return false;
    }
}