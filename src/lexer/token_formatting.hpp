#pragma once

#include "bu/utilities.hpp"
#include "token.hpp"


namespace lexer {

    auto token_description(Token::Type) -> std::string_view;

}


DECLARE_FORMATTER_FOR(lexer::Token::Type);
DECLARE_FORMATTER_FOR(lexer::Token);