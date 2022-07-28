#include "bu/utilities.hpp"
#include "parser_internals.hpp"


namespace {

    using namespace parser;


    auto extract_qualified_upper_name(ast::Root_qualifier&& root, Parse_context& context)
        -> ast::Type::Variant
    {
        auto* const anchor = context.pointer;
        auto name = extract_qualified(std::move(root), context);

        if (name.primary_name.is_upper) {
            if (auto template_arguments = parse_template_arguments(context)) {
                return ast::type::Template_application {
                    std::move(*template_arguments),
                    std::move(name)
                };
            }
            else {
                return ast::type::Typename { std::move(name) };
            }
        }
        else {
            context.error(
                { anchor, context.pointer },
                "Expected a typename, but found a lowercase identifier"
            );
        }
    }


    template <class T>
    constexpr Extractor extract_primitive = +[](Parse_context&)
        -> ast::Type::Variant
    {
        return ast::type::Primitive<T> {};
    };

    constexpr Extractor extract_wildcard = +[](Parse_context&)
        -> ast::Type::Variant
    {
        return ast::type::Wildcard {};
    };

    constexpr Extractor extract_typename = +[](Parse_context& context)
        -> ast::Type::Variant
    {
        context.retreat();
        return extract_qualified_upper_name({ std::monostate {} }, context);
    };

    constexpr Extractor extract_global_typename = +[](Parse_context& context)
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

    constexpr Extractor extract_array_or_slice = +[](Parse_context& context)
        -> ast::Type::Variant
    {
        auto element_type = extract_type(context);

        auto type = [&]() -> ast::Type::Variant {
            if (context.try_consume(Token::Type::semicolon)) {
                if (auto length = parse_expression(context)) {
                    return ast::type::Array {
                        .element_type = std::move(element_type),
                        .length       = std::move(*length)
                    };
                }
                else {
                    context.error_expected("the array length", "Remove the ';' if a slice type was intended");
                }
            }
            else {
                return ast::type::Slice { std::move(element_type) };
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
                    context.error_expected("the function return type");
                }
            }
            else {
                context.error_expected("a ':' followed by the function return type");
            }
        }
        else {
            context.error_expected("a parenthesized list of argument types");
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
            context.error_expected("a parenthesized expression");
        }
    };

    constexpr Extractor extract_instance_of = +[](Parse_context& context)
        -> ast::Type::Variant
    {
        return ast::type::Instance_of { extract_class_references(context) };
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

    constexpr Extractor extract_for_all = +[](Parse_context& context)
        -> ast::Type::Variant
    {
        if (auto parameters = parse_template_parameters(context)) {
            return ast::type::For_all {
                .parameters = std::move(*parameters),
                .body       = extract_type(context)
            };
        }
        else {
            context.error_expected("'[' followed by a comma-separated list of template parameters");
        }
    };


    auto parse_normal_type(Parse_context& context) -> std::optional<ast::Type> {
        switch (context.extract().type) {
        case Token::Type::string_type:
            return extract_primitive<lexer::String>(context);
        case Token::Type::integer_type:
            return extract_primitive<bu::Isize>(context);
        case Token::Type::floating_type:
            return extract_primitive<bu::Float>(context);
        case Token::Type::character_type:
            return extract_primitive<bu::Char>(context);
        case Token::Type::boolean_type:
            return extract_primitive<bool>(context);
        case Token::Type::underscore:
            return extract_wildcard(context);
        case Token::Type::paren_open:
            return extract_tuple(context);
        case Token::Type::bracket_open:
            return extract_array_or_slice(context);
        case Token::Type::fn:
            return extract_function(context);
        case Token::Type::type_of:
            return extract_type_of(context);
        case Token::Type::inst:
            return extract_instance_of(context);
        case Token::Type::ampersand:
            return extract_reference(context);
        case Token::Type::asterisk:
            return extract_pointer(context);
        case Token::Type::upper_name:
        case Token::Type::lower_name:
            return extract_typename(context);
        case Token::Type::double_colon:
            return extract_global_typename(context);
        case Token::Type::for_:
            return extract_for_all(context);
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

        if (name.primary_name.is_upper) {
            auto template_arguments = parse_template_arguments(context);

            type = ast::Type {
                [&]() -> ast::Type::Variant {
                    if (template_arguments) {
                        return ast::type::Template_application {
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
            type = std::get<bu::Wrapper<ast::Type>>(name.root_qualifier.value).kill();
        }
    }

    return type;
}