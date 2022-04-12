#include "bu/utilities.hpp"
#include "parser_internals.hpp"


namespace {

    using namespace parser;


    auto extract_qualified_upper_name(ast::Root_qualifier&& root, Parse_context& context)
        -> ast::Type::Variant
    {
        auto* const anchor = context.pointer;
        auto name = extract_qualified(std::move(root), context);

        if (name.primary_qualifier.is_upper) {
            if (auto template_arguments = parse_template_arguments(context)) {
                return ast::type::Template_instantiation {
                    std::move(*template_arguments),
                    std::move(name)
                };
            }
            else {
                return ast::type::Typename { std::move(name) };
            }
        }
        else {
            throw context.error(
                { anchor, context.pointer },
                "Expected a typename, but found a lowercase identifier"
            );
        }
    }


    constexpr Extractor parse_typename = +[](Parse_context& context)
        -> ast::Type::Variant
    {
        static constexpr std::hash<std::string_view> hasher;

        static auto const
            integer   = hasher("Int"),
            floating  = hasher("Float"),
            character = hasher("Char"),
            boolean   = hasher("Bool"),
            string    = hasher("String");

        auto const id = context.previous().as_identifier();
        auto const hash = id.hash();

        if (hash == integer) {
            return ast::type::Integer {};
        }
        else if (hash == floating) {
            return ast::type::Floating {};
        }
        else if (hash == character) {
            return ast::type::Character {};
        }
        else if (hash == boolean) {
            return ast::type::Boolean {};
        }
        else if (hash == string) {
            return ast::type::String {};
        }
        else {
            context.retreat();
            return extract_qualified_upper_name({ std::monostate {} }, context);
        }
    };

    constexpr Extractor parse_lower_qualified_typename = +[](Parse_context& context)
        -> ast::Type::Variant
    {
        context.retreat();
        return extract_qualified_upper_name({ std::monostate {} }, context);
    };

    constexpr Extractor parse_global_typename = +[](Parse_context& context)
        -> ast::Type::Variant
    {
        return extract_qualified_upper_name({ ast::Root_qualifier::Global{} }, context);
    };

    constexpr Extractor extract_tuple = +[](Parse_context& context)
        -> ast::Type::Variant
    {
        auto types = extract_comma_separated_zero_or_more<parse_type, "a type">(context);
        context.consume_required(Token::Type::paren_close);
        if (types.size() == 1) {
            return std::move(types.front().value);
        }
        else {
            return ast::type::Tuple { std::move(types) };
        }
    };

    constexpr Extractor extract_array_or_list = +[](Parse_context& context)
        -> ast::Type::Variant
    {
        auto element_type = extract_type(context);

        auto type = [&]() -> ast::Type::Variant {
            if (context.try_consume(Token::Type::semicolon)) {
                if (auto length = parse_expression(context)) {
                    return ast::type::Array {
                        std::move(element_type),
                        std::move(*length)
                    };
                }
                else {
                    throw context.expected("the array length");
                }
            }
            else {
                return ast::type::List { std::move(element_type) };
            }
        }();

        context.consume_required(Token::Type::bracket_close);
        return type;
    };

    constexpr Extractor extract_function = +[](Parse_context& context)
        -> ast::Type::Variant
    {
        if (context.try_consume(Token::Type::paren_open)) {
            static constexpr auto parse_argument_types =
                extract_comma_separated_zero_or_more<parse_type, "a type">;

            auto argument_types = parse_argument_types(context);
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
    };

    constexpr Extractor extract_type_of = +[](Parse_context& context)
        -> ast::Type::Variant
    {
        if (context.try_consume(Token::Type::paren_open)) {
            auto expression = extract_expression(context);
            context.consume_required(Token::Type::paren_close);
            return ast::type::Type_of { std::move(expression) };
        }
        else {
            throw context.expected("a parenthesized expression");
        }
    };

    constexpr Extractor extract_reference = +[](Parse_context& context)
        -> ast::Type::Variant
    {
        auto const mutability = extract_mutability(context);
        return ast::type::Reference { extract_type(context), mutability };
    };

    constexpr Extractor extract_pointer = +[](Parse_context& context)
        -> ast::Type::Variant
    {
        auto mutability = extract_mutability(context);
        return ast::type::Pointer { extract_type(context), std::move(mutability) };
    };


    auto parse_normal_type(Parse_context& context) -> std::optional<ast::Type> {
        switch (context.extract().type) {
        case Token::Type::paren_open:
            return extract_tuple(context);
        case Token::Type::bracket_open:
            return extract_array_or_list(context);
        case Token::Type::fn:
            return extract_function(context);
        case Token::Type::type_of:
            return extract_type_of(context);
        case Token::Type::ampersand:
            return extract_reference(context);
        case Token::Type::asterisk:
            return extract_pointer(context);
        case Token::Type::upper_name:
            return parse_typename(context);
        case Token::Type::lower_name:
            return parse_lower_qualified_typename(context);
        case Token::Type::double_colon:
            return parse_global_typename(context);
        default:
            context.retreat();
            return std::nullopt;
        }
    }

}


auto parser::parse_type(Parse_context& context) -> std::optional<ast::Type> {
    auto        type   = parse_normal_type(context);
    auto* const anchor = context.pointer;

    if (type && context.try_consume(Token::Type::double_colon)) {
        auto name = extract_qualified({ std::move(*type) }, context);

        if (name.primary_qualifier.is_upper) {
            auto template_arguments = parse_template_arguments(context);

            type = ast::Type {
                [&]() -> ast::Type::Variant {
                    if (template_arguments) {
                        return ast::type::Template_instantiation {
                            std::move(*template_arguments),
                            std::move(name)
                        };
                    }
                    else {
                        return ast::type::Typename { std::move(name) };
                    }
                }(),
                anchor->source_view + context.pointer[-1].source_view
            };
        }
        else {
            // Not a qualified type, retreat
            context.pointer = anchor;
            type = std::move(std::get<ast::Type>(name.root_qualifier->value));
        }
    }

    return type;
}