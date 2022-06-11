#include "bu/utilities.hpp"
#include "token_formatting.hpp"


auto lexer::token_description(Token::Type const type) -> std::string_view {
    switch (type) {
    case Token::Type::dot:           return "a '.'";
    case Token::Type::comma:         return "a ','";
    case Token::Type::colon:         return "a ':'";
    case Token::Type::semicolon:     return "a ';'";
    case Token::Type::double_colon:  return "a '::'";
    case Token::Type::ampersand:     return "a '&'";
    case Token::Type::asterisk:      return "a '*'";
    case Token::Type::plus:          return "a '+'";
    case Token::Type::question:      return "a '?'";
    case Token::Type::equals:        return "a '='";
    case Token::Type::pipe:          return "a '|'";
    case Token::Type::lambda:        return "a '\\'";
    case Token::Type::right_arrow:   return "a '->'";
    case Token::Type::hole:          return "a hole";
    case Token::Type::paren_open:    return "a '('";
    case Token::Type::paren_close:   return "a ')'";
    case Token::Type::brace_open:    return "a '{'";
    case Token::Type::brace_close:   return "a '}'";
    case Token::Type::bracket_open:  return "a '['";
    case Token::Type::bracket_close: return "a ']'";

    case Token::Type::let:
    case Token::Type::mut:
    case Token::Type::immut:
    case Token::Type::if_:
    case Token::Type::else_:
    case Token::Type::for_:
    case Token::Type::in:
    case Token::Type::while_:
    case Token::Type::loop:
    case Token::Type::continue_:
    case Token::Type::break_:
    case Token::Type::match:
    case Token::Type::ret:
    case Token::Type::fn:
    case Token::Type::as:
    case Token::Type::enum_:
    case Token::Type::struct_:
    case Token::Type::class_:
    case Token::Type::inst:
    case Token::Type::alias:
    case Token::Type::import_:
    case Token::Type::export_:
    case Token::Type::module_:
    case Token::Type::size_of:
    case Token::Type::type_of:
    case Token::Type::meta:
    case Token::Type::where:
    case Token::Type::dyn:
    case Token::Type::pub:
        return "a keyword";

    case Token::Type::underscore:    return "a wildcard pattern";
    case Token::Type::lower_name:    return "an uncapitalized identifier";
    case Token::Type::upper_name:    return "a capitalized identifier";
    case Token::Type::operator_name: return "an operator";
    case Token::Type::string:        return "a string literal";
    case Token::Type::integer:       return "an integer literal";
    case Token::Type::floating:      return "a floating-point literal";
    case Token::Type::character:     return "a character literal";
    case Token::Type::boolean:       return "a boolean literal";
    case Token::Type::end_of_input:  return "the end of input";

    case Token::Type::string_type:
    case Token::Type::integer_type:
    case Token::Type::floating_type:
    case Token::Type::character_type:
    case Token::Type::boolean_type:
        return "a primitive typename";

    default:
        bu::abort(std::format("Unimplemented for {}", type));
    }
}


DEFINE_FORMATTER_FOR(lexer::Token::Type) {
    constexpr auto strings = std::to_array<std::string_view>({
        ".", ",", ":", ";", "::", "&", "*", "+", "?", "=", "|", "\\", "->", "???", "(", ")", "{", "}", "[", "]",

        "let", "mut", "immut", "if", "else", "elif", "for", "in", "while", "loop", "continue",
        "break", "match", "ret", "fn", "as", "enum", "struct", "class", "inst", "impl", "alias",
        "namespace", "import", "export", "module", "size_of", "type_of", "meta", "where", "dyn", "pub",

        "underscore", "lower", "upper", "op",

        "str", "int", "float", "char", "bool",

        "String", "Int", "Float", "Char", "Bool",

        "end of input"
    });
    static_assert(strings.size() == static_cast<bu::Usize>(lexer::Token::Type::_token_type_count));
    return std::format_to(context.out(), "{}", strings[static_cast<bu::Usize>(value)]);
}


DEFINE_FORMATTER_FOR(lexer::Token) {
    if (std::holds_alternative<std::monostate>(value.value)) {
        return std::format_to(context.out(), "'{}'", value.type);
    }
    else {
        return std::format_to(context.out(), "({}: '{}')", value.type, value.value);
    }
}