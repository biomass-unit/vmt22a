#include "bu/utilities.hpp"
#include "bu/uninitialized.hpp"
#include "parser_internals.hpp"


namespace {

    using namespace parser;


    auto parse_qualified(lexer::Token* anchor, ast::Root_qualifier&& root, Parse_context& context)
        -> std::optional<ast::Type>
    {
        auto qualifiers = extract_qualifiers(context);

        if (qualifiers.empty()) {
            using R = std::optional<ast::Type>;

            return std::visit(bu::Overload {
                [](std::monostate) -> R {
                    bu::unimplemented();
                },
                [&](ast::Root_qualifier::Global) -> R {
                    throw context.expected("a type");
                },
                [&](lexer::Identifier) -> R {
                    context.pointer = anchor;
                    return std::nullopt;
                },
                [](ast::Type& type) -> R {
                    return std::move(type);
                }
            }, root.value);
        }
        else {
            auto back = std::move(qualifiers.back().value);
            qualifiers.pop_back();

            static_assert(std::variant_size_v<decltype(back)> == 2);

            if (auto* upper = std::get_if<ast::Middle_qualifier::Upper>(&back)) {
                ast::Qualified_name name {
                    .root_qualifier = std::move(root),
                    .qualifiers     = std::move(qualifiers),
                    .identifier     = upper->name
                };
                if (upper->template_arguments) {
                    return ast::type::Template_instantiation {
                        std::move(*upper->template_arguments),
                        std::move(name)
                    };
                }
                else {
                    return ast::type::Typename { std::move(name) };
                }
            }
            else {
                context.pointer = anchor;
                return std::nullopt;
            }
        }
    }


    auto parse_typename(Parse_context& context) -> std::optional<ast::Type> {
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
            return *ast::type::integer;
        }
        else if (hash == floating) {
            return *ast::type::floating;
        }
        else if (hash == character) {
            return *ast::type::character;
        }
        else if (hash == boolean) {
            return *ast::type::boolean;
        }
        else if (hash == string) {
            return *ast::type::string;
        }
        else {
            context.retreat();
            return parse_qualified(context.pointer, {}, context);
        }
    }

    auto parse_lower_qualified_typename(Parse_context& context) -> std::optional<ast::Type> {
        context.retreat();
        return parse_qualified(context.pointer, {}, context);
    }

    auto parse_global_typename(Parse_context& context) -> std::optional<ast::Type> {
        return parse_qualified(&context.previous(), { ast::Root_qualifier::Global{} }, context);
    }

    auto extract_tuple(Parse_context& context) -> ast::Type {
        auto types = extract_comma_separated_zero_or_more<parse_type, "a type">(context);
        context.consume_required(Token::Type::paren_close);
        if (types.size() == 1) {
            return std::move(types.front());
        }
        else {
            return ast::type::Tuple { std::move(types) };
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
        return std::move(*type);
    }

    auto extract_function(Parse_context& context) -> ast::Type {
        if (context.try_consume(Token::Type::paren_open)) {
            static constexpr auto parse_argument_types =
                extract_comma_separated_zero_or_more<parse_wrapped<parse_type>, "a type">;

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

    auto extract_reference(Parse_context& context) -> ast::Type {
        auto const mutability = extract_mutability(context);
        return ast::type::Reference { extract_type(context), mutability };
    }

    auto extract_pointer(Parse_context& context) -> ast::Type {
        auto mutability = extract_mutability(context);
        return ast::type::Pointer { extract_type(context), std::move(mutability) };
    }


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
    auto type = parse_normal_type(context);
    auto anchor = context.pointer;

    if (type && context.pointer->type == Token::Type::double_colon) {
        if (auto qualified_type = parse_qualified(anchor, { *type }, context)) {
            type = qualified_type;
        }
    }

    return type;
}