#include "bu/utilities.hpp"
#include "token_formatting.hpp"


DEFINE_FORMATTER_FOR(lexer::Token::Type) {
    constexpr auto strings = std::to_array<std::string_view>({
        ".", ",", ":", ";", "::", "&", "*", "+", "?", "=", "|", "->", "(", ")", "{{", "}}", "[", "]",
        "let", "mut", "immut", "if", "else", "elif", "for", "in", "while", "loop", "continue", "break", "match", "ret", "fn", "as", "data", "struct", "class", "inst", "alias", "import", "module", "size_of", "type_of", "meta", "where",
        "underscore", "lower", "upper", "op", "str", "int", "float", "char", "bool", "end of input"
    });
    static_assert(strings.size() == lexer::Token::type_count);
    return std::format_to(context.out(), strings[static_cast<bu::Usize>(value)]);
}


DEFINE_FORMATTER_FOR(lexer::Token) {
    if (std::holds_alternative<std::monostate>(value.value)) {
        return std::format_to(context.out(), "'{}'", value.type);
    }
    else {
        return std::format_to(context.out(), "({}: '{}')", value.type, value.value);
    }
}