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
        inline auto try_extract(Token::Type type) noexcept -> Token* {
            return pointer->type == type ? pointer++ : nullptr;
        }
        inline auto consume_required(Token::Type type) -> void {
            if (pointer->type == type) {
                ++pointer;
            }
            else {
                bu::abort("required token not found");
            }
        }
    };

    auto parse_expression(Parse_context&) -> std::optional<ast::Expression>;
    auto parse_pattern   (Parse_context&) -> std::optional<ast::Pattern>;
    auto parse_type      (Parse_context&) -> std::optional<ast::Type>;

}