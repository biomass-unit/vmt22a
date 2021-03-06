#pragma once

#include "bu/utilities.hpp"
#include "bu/diagnostics.hpp"
#include "lexer/lexer.hpp"
#include "ast/ast.hpp"


namespace parser {

    using lexer::Token;


    struct Parse_context {
        Token                   * start;
        Token                   * pointer;
        bu::Source              & source;
        bu::diagnostics::Builder& diagnostics;

        explicit Parse_context(lexer::Tokenized_source& ts) noexcept
            : start       { ts.tokens.data() }
            , pointer     { start }
            , source      { ts.source }
            , diagnostics { ts.diagnostics } {}

        auto is_finished() const noexcept -> bool {
            return pointer->type == Token::Type::end_of_input;
        }

        auto try_extract(Token::Type const type) noexcept -> Token* {
            return pointer->type == type ? pointer++ : nullptr;
        }

        auto extract() noexcept -> Token& {
            return *pointer++;
        }

        auto previous() noexcept -> Token& {
            assert(pointer != start);
            return pointer[-1];
        }

        auto consume_required(Token::Type const type) -> void {
            if (pointer->type == type) {
                ++pointer;
            }
            else {
                error_expected("'{}'"_format(type));
            }
        }

        auto try_consume(Token::Type const type) noexcept -> bool {
            if (pointer->type == type) {
                ++pointer;
                return true;
            }
            else {
                return false;
            }
        }

        auto retreat() noexcept -> void {
            --pointer;
        }

        [[noreturn]]
        auto error(
            bu::Source_view                    const erroneous_view,
            bu::diagnostics::Message_arguments const arguments) const -> void
        {
            diagnostics.emit_simple_error(arguments.add_source_info(source, erroneous_view));
        }

        [[noreturn]]
        auto error(
            std::span<Token const>             const span,
            bu::diagnostics::Message_arguments const arguments) const -> void
        {
            error(span.front().source_view + span.back().source_view, arguments);
        }

        [[noreturn]]
        auto error(
            std::span<Token const> const span,
            std::string_view       const message) const -> void
        {
            error(span, { .message = message });
        }

        [[noreturn]]
        auto error(bu::diagnostics::Message_arguments const arguments) const -> void {
            error({ pointer, pointer + 1 }, arguments);
        }

        [[noreturn]]
        auto error_expected(
            std::span<Token const>          const span,
            std::string_view                const expectation,
            std::optional<std::string_view> const help = std::nullopt) const -> void
        {
            error(span, {
                .message           = "Expected {}, but found {}",
                .message_arguments = std::make_format_args(expectation, lexer::token_description(pointer->type)),
                .help_note         = help
            });
        }

        [[noreturn]]
        auto error_expected(
            std::string_view                const expectation,
            std::optional<std::string_view> const help = std::nullopt) const -> void
        {
            error_expected({ pointer, pointer + 1 }, expectation, help);
        }
    };


    template <class P>
    concept parser = requires (P p, Parse_context context) {
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
            context.error_expected(description.view());
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
                    context.error_expected("a closing '{}'"_format(close));
                }
            }
            else {
                context.error_expected(description.view());
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
                    context.error_expected(description.view());
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

    constexpr auto extract_expression = extract_required<parse_expression, "an expression">;
    constexpr auto extract_pattern    = extract_required<parse_pattern   , "a pattern"    >;
    constexpr auto extract_type       = extract_required<parse_type      , "a type"       >;

    auto parse_top_level_pattern(Parse_context&) -> std::optional<ast::Pattern>;

    auto parse_block_expression(Parse_context&) -> std::optional<ast::Expression>;

    auto parse_template_arguments(Parse_context&) -> std::optional<std::vector<ast::Template_argument>>;

    auto parse_template_parameters(Parse_context&) -> std::optional<std::vector<ast::Template_parameter>>;

    auto extract_function_parameters(Parse_context&) -> std::vector<ast::Function_parameter>;

    auto extract_qualified(ast::Root_qualifier&&, Parse_context&) -> ast::Qualified_name;

    auto extract_mutability(Parse_context&) -> ast::Mutability;

    auto parse_class_reference(Parse_context&) -> std::optional<ast::Class_reference>;

    auto extract_class_references(Parse_context&) -> std::vector<ast::Class_reference>;


    template <Token::Type id_type>
    auto extract_id(Parse_context& context, std::string_view const description)
        -> lexer::Identifier
    {
        if (auto* const token = context.try_extract(id_type)) {
            return token->as_identifier();
        }
        else {
            context.error_expected(description);
        }
    }

    constexpr auto extract_lower_id = extract_id<Token::Type::lower_name>;
    constexpr auto extract_upper_id = extract_id<Token::Type::upper_name>;


    template <Token::Type id_type>
    auto parse_id(Parse_context& context) -> std::optional<lexer::Identifier> {
        if (auto* const token = context.try_extract(id_type)) {
            return token->as_identifier();
        }
        else {
            return std::nullopt;
        }
    }

    constexpr auto parse_lower_id = parse_id<Token::Type::lower_name>;
    constexpr auto parse_upper_id = parse_id<Token::Type::upper_name>;


    template <Token::Type id_type>
    auto parse_name(Parse_context& context)
        -> std::optional<ast::Name>
    {
        if (auto* const token = context.try_extract(id_type)) {
            return ast::Name {
                .identifier  = token->as_identifier(),
                .is_upper    = id_type == Token::Type::upper_name,
                .source_view = token->source_view
            };
        }
        else {
            return std::nullopt;
        }
    }

    constexpr auto parse_lower_name = parse_name<Token::Type::lower_name>;
    constexpr auto parse_upper_name = parse_name<Token::Type::upper_name>;


    template <Token::Type id_type>
    auto extract_name(Parse_context& context, std::string_view const description)
        -> ast::Name
    {
        if (auto name = parse_name<id_type>(context)) {
            return std::move(*name);
        }
        else {
            context.error_expected(description);
        }
    }

    constexpr auto extract_lower_name = extract_name<Token::Type::lower_name>;
    constexpr auto extract_upper_name = extract_name<Token::Type::upper_name>;


    inline auto make_source_view(Token const* const first, Token const* const last)
        noexcept -> bu::Source_view
    {
        assert(first <= last);
        return first->source_view + last->source_view;
    }


    template <class Node, std::optional<typename Node::Variant> (*parse)(Parse_context&)>
    auto parse_node(Parse_context& context) -> std::optional<Node> {
        Token const* const anchor = context.pointer;

        if (auto node_value = parse(context)) {
            return Node {
                .value       = std::move(*node_value),
                .source_view = anchor->source_view + context.pointer[-1].source_view
            };
        }
        else {
            return std::nullopt;
        }
    }

    template <class Node, Node::Variant (*extract)(Parse_context&)>
    auto extract_node(Parse_context& context) -> Node {
        Token const* const     anchor = context.pointer;
        typename Node::Variant value  = extract(context);
        return {
            .value       = std::move(value),
            .source_view = anchor->source_view + context.pointer[-1].source_view
        };
    }

}