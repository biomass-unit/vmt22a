#include "bu/utilities.hpp"
#include "token_formatting.hpp"


DEFINE_FORMATTER_FOR(lexer::Token::Type) {
    constexpr auto strings = std::to_array<std::string_view>({
        ".", ",", ":", ";", "::", "...", "&", "|", "(", ")", "{{", "}}", "[", "]",
        "let", "mut", "if", "else", "for", "in", "while", "loop", "continue", "break", "match", "ret", "fn", "as", "data", "struct", "class", "inst", "alias", "import", "size_of", "type_of", "meta", "mod", "underscore", "lower", "upper", "op", "str", "int", "float", "char", "bool", "end of input"
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