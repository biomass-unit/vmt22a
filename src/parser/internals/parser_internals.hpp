#pragma once

#include "bu/utilities.hpp"
#include "bu/source.hpp"
#include "lexer/token.hpp"
#include "ast/ast.hpp"


namespace parser {

    using lexer::Token;

    struct Parse_context {
        Token* pointer;
        bu::Source* source;

        inline auto is_finished() const noexcept -> bool {
            return pointer->type == Token::Type::end_of_input;
        }
        inline auto try_extract(Token::Type const type) noexcept -> Token* {
            return pointer->type == type ? pointer++ : nullptr;
        }
        inline auto extract() noexcept -> Token& {
            return *pointer++;
        }
        inline auto consume_required(Token::Type const type) -> void {
            if (pointer->type == type) {
                ++pointer;
            }
            else {
                bu::unimplemented();
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
    };

    
    auto parse_expression(Parse_context&) -> std::optional<ast::Expression>;
    auto parse_pattern   (Parse_context&) -> std::optional<ast::Pattern>;
    auto parse_type      (Parse_context&) -> std::optional<ast::Type>;


    template <class P>
    concept parser = requires (P p, Parse_context& context) {
        { p(context) } -> bu::instance_of<std::optional>;
    };

    template <parser auto parse>
    using Parse_result = std::invoke_result_t<decltype(parse), Parse_context&>::value_type;


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

    template <parser auto parse>
    auto extract_comma_separated_zero_or_more(Parse_context& context)
        -> std::vector<Parse_result<parse>>
    {
        std::vector<Parse_result<parse>> vector;

        if (auto head = parse(context)) {
            vector.push_back(std::move(*head));

            while (context.try_consume(Token::Type::comma)) {
                if (auto element = parse(context)) {
                    vector.push_back(std::move(*element));
                }
                else {
                    bu::abort("expected an element after the comma");
                }
            }
        }

        return vector;
    }

    template <parser auto parse>
    auto parse_comma_separated_one_or_more(Parse_context& context)
        -> std::optional<std::vector<Parse_result<parse>>>
    {
        auto vector = extract_comma_separated_zero_or_more<parse>(context);

        if (!vector.empty()) {
            return vector;
        }
        else {
            bu::unimplemented();
        }
    }

}