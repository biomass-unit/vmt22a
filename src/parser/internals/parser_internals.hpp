#pragma once

#include "bu/utilities.hpp"
#include "bu/source.hpp"
#include "bu/textual_error.hpp"
#include "lexer/lexer.hpp"
#include "lexer/token_formatting.hpp"
#include "ast/ast.hpp"


namespace parser {

    using lexer::Token;

    struct Parse_context {
        Token* start;
        Token* pointer;
        bu::Source* source;

        explicit Parse_context(lexer::Tokenized_source& ts) noexcept
            : start   { ts.tokens.data() }
            , pointer { start }
            , source  { &ts.source } {}

        inline auto is_finished() const noexcept -> bool {
            return pointer->type == Token::Type::end_of_input;
        }

        inline auto try_extract(Token::Type const type) noexcept -> Token* {
            return pointer->type == type ? pointer++ : nullptr;
        }

        inline auto extract() noexcept -> Token& {
            return *pointer++;
        }

        inline auto previous() noexcept -> Token& {
            assert(pointer != start);
            return pointer[-1];
        }

        inline auto consume_required(Token::Type const type) -> void {
            if (pointer->type == type) {
                ++pointer;
            }
            else {
                throw expected(std::format("'{}'", type));
            }
        }

        inline auto try_consume(Token::Type const type) noexcept -> bool {
            if (pointer->type == type) {
                ++pointer;
                return true;
            }
            else {
                return false;
            }
        }

        inline auto error(std::string_view message, std::optional<std::string_view> help = std::nullopt) const -> bu::Textual_error {
            return bu::Textual_error { {
                .error_type     = bu::Textual_error::Type::parse_error,
                .erroneous_view = pointer->source_view,
                .file_view      = source->string(),
                .file_name      = source->name(),
                .message        = message,
                .help_note      = help
            } };
        }

        inline auto expected(std::string_view expectation, std::optional<std::string_view> help = std::nullopt) const -> bu::Textual_error {
            return error(std::format("Expected {}", expectation), help);
        }
    };


    template <class P>
    concept parser = requires (P p, Parse_context& context) {
        { p(context) } -> bu::instance_of<std::optional>;
    };

    template <parser auto p>
    using Parse_result = std::invoke_result_t<decltype(p), Parse_context&>::value_type;


    template <parser auto p, bu::Metastring description>
    auto extract_required(Parse_context& context) -> Parse_result<p> {
        if (auto result = p(context)) {
            return std::move(*result);
        }
        else {
            throw context.expected(description.view());
        }
    }

    template <parser auto p, parser auto... ps>
    auto parse_one_of(Parse_context& context) -> decltype(p(context)) {
        if (auto result = p(context)) {
            return result;
        }
        else {
            if constexpr (sizeof...(ps) != 0) {
                return parse_one_of<ps...>(context);
            }
            else {
                return std::nullopt;
            }
        }
    }


    template <parser auto p, bu::Metastring description, Token::Type open, Token::Type close>
    auto parse_surrounded(Parse_context& context) -> decltype(p(context)) {
        if (context.try_consume(open)) {
            if (auto result = p(context)) {
                if (context.try_consume(close)) {
                    return result;
                }
                else {
                    throw context.expected(std::format("a closing '{}'", close));
                }
            }
            else {
                throw context.expected(description.view());
            }
        }
        else {
            return std::nullopt;
        }
    }


    template <parser auto p, bu::Metastring description>
    constexpr auto parenthesized =
        parse_surrounded<p, description, Token::Type::paren_open, Token::Type::paren_close>;

    template <parser auto p, bu::Metastring description>
    constexpr auto braced =
        parse_surrounded<p, description, Token::Type::brace_open, Token::Type::brace_close>;

    template <parser auto p, bu::Metastring description>
    constexpr auto bracketed =
        parse_surrounded<p, description, Token::Type::brace_open, Token::Type::bracket_close>;


    template <parser auto p, Token::Type separator, bu::Metastring description>
    auto extract_separated_zero_or_more(Parse_context& context)
        -> std::vector<Parse_result<p>>
    {
        std::vector<Parse_result<p>> vector;

        if (auto head = p(context)) {
            vector.push_back(std::move(*head));

            while (context.try_consume(separator)) {
                if (auto element = p(context)) {
                    vector.push_back(std::move(*element));
                }
                else {
                    throw context.expected(description.view());
                }
            }
        }

        return vector;
    }

    template <parser auto p, Token::Type separator, bu::Metastring description>
    auto parse_separated_one_or_more(Parse_context& context)
        -> std::optional<std::vector<Parse_result<p>>>
    {
        auto vector = extract_separated_zero_or_more<p, separator, description>(context);

        if (!vector.empty()) {
            return vector;
        }
        else {
            return std::nullopt;
        }
    }


    template <parser auto p, bu::Metastring description>
    constexpr auto extract_comma_separated_zero_or_more =
        extract_separated_zero_or_more<p, Token::Type::comma, description>;

    template <parser auto p, bu::Metastring description>
    constexpr auto parse_comma_separated_one_or_more =
        parse_separated_one_or_more<p, Token::Type::comma, description>;


    auto parse_expression(Parse_context&) -> std::optional<ast::Expression>;
    auto parse_pattern   (Parse_context&) -> std::optional<ast::Pattern   >;
    auto parse_type      (Parse_context&) -> std::optional<ast::Type      >;

    inline constexpr auto extract_expression = extract_required<parse_expression, "an expression">;
    inline constexpr auto extract_pattern    = extract_required<parse_pattern   , "a pattern"    >;
    inline constexpr auto extract_type       = extract_required<parse_type      , "a type"       >;

    auto parse_compound_expression(Parse_context&) -> std::optional<ast::Expression>;


    template <Token::Type id_type, bu::Metastring description>
    auto extract_id(Parse_context& context) -> lexer::Identifier {
        if (auto token = context.try_extract(id_type)) {
            return token->as_identifier();
        }
        else {
            throw context.expected(description.view());
        }
    }

    template <bu::Metastring description>
    constexpr auto extract_lower_id = extract_id<Token::Type::lower_name, description>;
    template <bu::Metastring description>
    constexpr auto extract_upper_id = extract_id<Token::Type::upper_name, description>;


    template <Token::Type id_type>
    auto parse_id(Parse_context& context) -> std::optional<lexer::Identifier> {
        if (auto token = context.try_extract(id_type)) {
            return token->as_identifier();
        }
        else {
            return std::nullopt;
        }
    }

    inline constexpr auto parse_lower_id = parse_id<Token::Type::lower_name>;
    inline constexpr auto parse_upper_id = parse_id<Token::Type::upper_name>;


    template <parser auto parse>
    auto parse_and_add_source_view(Parse_context& context) {
        auto const anchor = context.pointer;
        auto result = parse(context);

        if (result) {
            auto const view = context.pointer->source_view;
            result->source_view = { anchor->source_view.data(), view.data() + view.size() };
        }
        return result;
    }

}