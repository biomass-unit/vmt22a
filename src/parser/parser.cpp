#include "bu/utilities.hpp"
#include "bu/uninitialized.hpp"
#include "parser.hpp"
#include "internals/parser_internals.hpp"


namespace {

    using namespace parser;


    thread_local ast::Namespace* current_namespace;


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

    auto extract_function(Parse_context& context) -> void {
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

                (*current_namespace)->function_definitions.push_back(
                    ast::definition::Function {
                        .body        = std::move(*body),
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


    auto parse_struct_member(Parse_context& context)
        -> std::optional<ast::definition::Struct::Member>
    {
        if (auto name = parse_lower_id(context)) {
            context.consume_required(Token::Type::colon);
            return ast::definition::Struct::Member {
                *name,
                extract_type(context)
            };
        }
        else {
            return std::nullopt;
        }
    }

    auto extract_struct(Parse_context& context) -> void {
        if (auto name = parse_upper_id(context)) {
            constexpr auto parse_members =
                parse_comma_separated_one_or_more<parse_struct_member, "a struct member">;

            context.consume_required(Token::Type::equals);
            if (auto members = parse_members(context)) {
                (*current_namespace)->struct_definitions.push_back(
                    ast::definition::Struct {
                        .members = std::move(*members),
                        .name    = *name
                    }
                );
            }
            else {
                throw context.expected("one or more struct members");
            }
        }
        else {
            throw context.expected("a struct name");
        }
    }


    auto parse_data_constructor(Parse_context& context)
        -> std::optional<ast::definition::Data::Constructor>
    {
        if (auto name = parse_upper_id(context)) {
            std::optional<bu::Wrapper<ast::Type>> type;

            if (context.try_consume(Token::Type::paren_open)) {
                auto types = extract_comma_separated_zero_or_more<parse_type, "a type">(context);
                switch (types.size()) {
                case 0:
                    break;
                case 1:
                    type.emplace(std::move(types.front()));
                    break;
                default:
                    type.emplace(ast::type::Tuple { std::move(types) });
                }
                context.consume_required(Token::Type::paren_close);
            }

            return ast::definition::Data::Constructor { *name, type };
        }
        else {
            return std::nullopt;
        }
    }

    auto extract_data(Parse_context& context) -> void {
        if (auto name = parse_upper_id(context)) {
            constexpr auto parse_constructors =
                parse_separated_one_or_more<parse_data_constructor, Token::Type::pipe, "a data constructor">;

            context.consume_required(Token::Type::equals);
            if (auto constructors = parse_constructors(context)) {
                (*current_namespace)->data_definitions.push_back(
                    ast::definition::Data {
                        .constructors = std::move(*constructors),
                        .name         = *name
                    }
                );
            }
            else {
                throw context.expected("one or more data constructors");
            }
        }
        else {
            throw context.expected("a data name");
        }
    }


    auto parse_definition(Parse_context&) -> bool;

    auto extract_namespace(Parse_context& context) -> void {
        if (auto name = parse_lower_id(context)) {
            context.consume_required(Token::Type::brace_open);

            auto previous = ::current_namespace;
            auto child = previous->make_child(*name);

            ::current_namespace = &child;
            while (parse_definition(context));
            ::current_namespace = previous;

            context.consume_required(Token::Type::brace_close);
        }
        else {
            throw context.expected("a module name");
        }
    }


    auto parse_definition(Parse_context& context) -> bool {
        switch (context.extract().type) {
        case Token::Type::fn:
            extract_function(context);
            break;
        case Token::Type::struct_:
            extract_struct(context);
            break;
        case Token::Type::data:
            extract_data(context);
            break;
        case Token::Type::module:
            extract_namespace(context);
            break;
        default:
            --context.pointer;
            return false;
        }
        return true;
    }

}


auto parser::parse(lexer::Tokenized_source&& tokenized_source) -> ast::Module {
    Parse_context context { tokenized_source };
    ast::Namespace global_namespace { lexer::Identifier { std::string_view { "" } } };
    ::current_namespace = &global_namespace;

    while (parse_definition(context));

    if (!context.is_finished()) {
        throw context.expected("a definition");
    }

    return { std::move(tokenized_source.source), global_namespace };
}