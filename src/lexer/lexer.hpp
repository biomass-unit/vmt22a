#pragma once

#include "bu/utilities.hpp"
#include "token.hpp"


namespace lexer {

    [[nodiscard]]
    auto lex(std::string_view) -> std::vector<Token>;

    inline namespace literals {

        [[nodiscard]]
        auto operator"" _id(char const*, bu::Usize) noexcept -> Identifier;

    }

}