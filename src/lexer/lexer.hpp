#pragma once

#include "bu/utilities.hpp"
#include "bu/source.hpp"
#include "token.hpp"


namespace lexer {

    struct Tokenized_source {
        bu::Source source;
        std::vector<Token> tokens;
    };


    [[nodiscard]]
    auto lex(bu::Source&&) -> Tokenized_source;


    inline namespace literals {

        [[nodiscard]]
        auto operator"" _id(char const*, bu::Usize) noexcept -> Identifier;

    }

}