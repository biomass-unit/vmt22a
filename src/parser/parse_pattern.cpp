#include "bu/utilities.hpp"
#include "parser_internals.hpp"


namespace {

    using namespace parser;


    auto extract_wildcard(Parse_context&)
		-> ast::Pattern::Variant
	{
        return ast::pattern::Wildcard {};
    };

    template <class T>
    auto extract_literal(Parse_context& context)
		-> ast::Pattern::Variant
	{
        return ast::pattern::Literal<T> { context.previous().value_as<T>() };
    };

    auto extract_tuple(Parse_context& context)
		-> ast::Pattern::Variant
	{
        auto patterns = extract_comma_separated_zero_or_more<parse_pattern, "a pattern">(context);
        context.consume_required(Token::Type::paren_close);

        if (patterns.size() == 1) {
            return std::move(patterns.front().value);
        }
        else {
            return ast::pattern::Tuple { std::move(patterns) };
        }
    };

    auto extract_slice(Parse_context& context)
		-> ast::Pattern::Variant
	{
        static constexpr auto extract_elements =
            extract_comma_separated_zero_or_more<parse_pattern, "an element pattern">;

        auto patterns = extract_elements(context);

        if (context.try_consume(Token::Type::bracket_close)) {
            return ast::pattern::Slice { std::move(patterns) };
        }
        else if (patterns.empty()) {
            context.error_expected("a slice element pattern or a ']'");
        }
        else {
            context.error_expected("a ',' or a ']'");
        }
    };


    constexpr std::optional<ast::Pattern> (*parse_constructor_pattern)(Parse_context&) =
        parenthesized<parse_top_level_pattern, "a pattern">;


    auto parse_constructor_name(Parse_context& context) -> std::optional<ast::Qualified_name> {
        auto* const anchor = context.pointer;

        auto name = [&]() -> std::optional<ast::Qualified_name> {
            switch (context.pointer->type) {
            case Token::Type::lower_name:
            case Token::Type::upper_name:
                return extract_qualified({}, context);
            case Token::Type::double_colon:
                ++context.pointer;
                return extract_qualified({ ast::Root_qualifier::Global {} }, context);
            default:
                if (auto type = parse_type(context)) {
                    return extract_qualified({ std::move(*type) }, context);
                }
                else {
                    return std::nullopt;
                }
            }
        }();

        if (name && name->primary_name.is_upper) {
            context.error(
                { anchor, context.pointer },
                "Expected an enum constructor name, but found a capitalized identifier"
            );
        }

        return name;
    }


    auto extract_name(Parse_context& context)
		-> ast::Pattern::Variant
	{
        context.retreat();
        auto mutability = extract_mutability(context);

        std::optional<ast::Name> name;

        if (mutability.type == ast::Mutability::Type::immut) { // Mutability wasn't specified
            if (auto ctor_name = parse_constructor_name(context)) {
                if (!ctor_name->is_unqualified()) {
                    return ast::pattern::Constructor {
                        .name    = std::move(*ctor_name),
                        .pattern = parse_constructor_pattern(context).transform(bu::wrap)
                    };
                }
                else {
                    name = ctor_name->primary_name;
                }
            }
        }

        if (!name) {
            name = extract_lower_name(context, "a lowercase identifier");
        }

        return ast::pattern::Name {
            .value      = std::move(*name),
            .mutability = std::move(mutability)
        };
    };

    auto extract_qualified_constructor(Parse_context& context)
		-> ast::Pattern::Variant
	{
        context.retreat();

        if (auto name = parse_constructor_name(context)) {
            return ast::pattern::Constructor {
                .name    = std::move(*name),
                .pattern = parse_constructor_pattern(context).transform(bu::wrap)
            };
        }
        else {
            bu::todo(); // Unreachable?
        }
    };

    auto extract_constructor_shorthand(Parse_context& context)
		-> ast::Pattern::Variant
	{
        auto name = extract_lower_name(context, "an enum constructor name");
        return ast::pattern::Constructor_shorthand {
            .name    = std::move(name),
            .pattern = parse_constructor_pattern(context).transform(bu::wrap)
        };
    };


    auto parse_normal_pattern(Parse_context& context) -> std::optional<ast::Pattern::Variant> {
        switch (context.extract().type) {
        case Token::Type::underscore:
            return extract_wildcard(context);
        case Token::Type::integer:
            return extract_literal<bu::Isize>(context);
        case Token::Type::floating:
            return extract_literal<bu::Float>(context);
        case Token::Type::character:
            return extract_literal<bu::Char>(context);
        case Token::Type::boolean:
            return extract_literal<bool>(context);
        case Token::Type::string:
            return extract_literal<lexer::String>(context);
        case Token::Type::paren_open:
            return extract_tuple(context);
        case Token::Type::bracket_open:
            return extract_slice(context);
        case Token::Type::lower_name:
        case Token::Type::mut:
            return extract_name(context);
        case Token::Type::upper_name:
            return extract_qualified_constructor(context);
        case Token::Type::colon:
            return extract_constructor_shorthand(context);
        default:
            context.retreat();
            return std::nullopt;
        }
    }


    auto parse_potentially_aliased_pattern(Parse_context& context)
        -> std::optional<ast::Pattern::Variant>
    {
        if (auto pattern = parse_node<ast::Pattern, parse_normal_pattern>(context)) {
            if (context.try_consume(Token::Type::as)) {
                auto mutability = extract_mutability(context);
                auto alias      = extract_lower_name(context, "a pattern alias");

                return ast::pattern::As {
                    .name {
                        .value      = std::move(alias),
                        .mutability = std::move(mutability)
                    },
                    .pattern = std::move(*pattern)
                };
            }
            return std::move(pattern->value);
        }
        else {
            return std::nullopt;
        }
    }


    auto parse_potentially_guarded_pattern(Parse_context& context)
        -> std::optional<ast::Pattern::Variant>
    {
        if (auto pattern = parse_node<ast::Pattern, parse_potentially_aliased_pattern>(context)) {
            if (context.try_consume(Token::Type::if_)) {
                if (auto guard = parse_expression(context)) {
                    return ast::pattern::Guarded {
                        std::move(*pattern),
                        std::move(*guard)
                    };
                }
                else {
                    context.error_expected("a guard expression");
                }
            }
            else {
                return std::move(pattern->value);
            }
        }
        else {
            return std::nullopt;
        }
    }

}


auto parser::parse_pattern(Parse_context& context) -> std::optional<ast::Pattern> {
    return parse_node<ast::Pattern, parse_potentially_guarded_pattern>(context);
}