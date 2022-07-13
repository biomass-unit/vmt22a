#pragma once

#include "bu/utilities.hpp"
#include "bu/source.hpp"
#include "bu/diagnostics.hpp"
#include "token.hpp"


namespace lexer {

    struct Tokenized_source {
        bu::Source               source;
        std::vector<Token>       tokens;
        bu::diagnostics::Builder diagnostics;
    };


    [[nodiscard]]
    auto lex(bu::Source&&) -> Tokenized_source;


    inline namespace literals {

        [[nodiscard]]
        auto operator"" _id(char const*, bu::Usize) noexcept -> Identifier;

    }

}