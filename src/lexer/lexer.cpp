#include "bu/utilities.hpp"
#include "bu/textual_error.hpp"
#include "lexer.hpp"
#include "token_formatting.hpp"

#include <charconv>


namespace {

    using Type = lexer::Token::Type;

    struct Lex_context {
        std::vector<lexer::Token> tokens;
        bu::Source* source;
        char const* start;
        char const* stop;
        char const* pointer;
        char const* token_start = nullptr;

        explicit Lex_context(bu::Source& source) noexcept
            : source  { &source }
            , start   { source.string().data() }
            , stop    { start + source.string().size() }
            , pointer { start }
        {
            tokens.reserve(1024);
        }

        struct State {
            char const* pointer;
        };

        inline auto state() const noexcept -> State {
            return { pointer };
        }

        inline auto is_finished() const noexcept -> bool {
            return pointer == stop;
        }

        inline auto current() const noexcept -> char {
            return *pointer;
        }

        inline auto extract_current() noexcept -> char {
            return *pointer++;
        }

        inline auto consume(std::predicate<char> auto const predicate) noexcept -> void {
            for (; (pointer != stop) && predicate(*pointer); ++pointer);
        }

        inline auto extract(std::predicate<char> auto const predicate) noexcept -> std::string_view {
            auto const anchor = pointer;
            consume(predicate);
            return { anchor, pointer };
        }

        inline auto try_consume(char const c) noexcept -> bool {
            assert(c != '\n');

            if (*pointer == c) {
                ++pointer;
                return true;
            }
            else {
                return false;
            }
        }

        inline auto try_consume(std::string_view string) noexcept -> bool {
            auto ptr = pointer;

            for (char const character : string) {
                assert(character != '\n');
                if (*ptr++ != character) {
                    return false;
                }
            }

            pointer = ptr;
            return true;
        }

        inline auto success(lexer::Token&& token) noexcept -> std::true_type {
            token.source_view = { token_start, pointer };
            tokens.push_back(std::move(token));
            return {};
        }

        inline auto failure(State const state) noexcept -> std::false_type {
            pointer = state.pointer;
            return {};
        }

        inline auto error(std::string_view view, std::string_view message, std::optional<std::string_view> help = std::nullopt) const -> bu::Textual_error {
            return bu::Textual_error { {
                .error_type     = bu::Textual_error::Type::lexical_error,
                .erroneous_view = view,
                .file_view      = source->string(),
                .file_name      = source->name(),
                .message        = message,
                .help_note      = help
            } };
        }

        inline auto error(char const* location, std::string_view message, std::optional<std::string_view> help = std::nullopt) const -> bu::Textual_error {
            return error({ location, location + 1 }, message, help);
        }
    };


    template <char... cs>
    constexpr auto is_one_of(char const c) noexcept -> bool {
        return ((c == cs) || ...);
    }

    template <char... cs>
    constexpr auto is_not_one_of(char const c) noexcept -> bool {
        return ((c != cs) && ...);
    }

    template <char a, char b>
        requires (a < b)
    constexpr auto is_in_range(char const c) noexcept -> bool {
        return a <= c && c <= b;
    }

    template <std::predicate<char> auto... predicates>
    constexpr auto satisfies_one_of(char const c) noexcept -> bool {
        return (predicates(c) || ...);
    }

    constexpr auto is_space = is_one_of<' ', '\t', '\n'>;
    constexpr auto is_digit = is_in_range<'0', '9'>;
    constexpr auto is_lower = is_in_range<'a', 'z'>;
    constexpr auto is_upper = is_in_range<'A', 'Z'>;
    constexpr auto is_alpha = satisfies_one_of<is_lower, is_upper>;
    constexpr auto is_alnum = satisfies_one_of<is_alpha, is_digit>;

    auto new_id(std::string_view const id) noexcept {
        return lexer::Identifier { id, lexer::Identifier::guaranteed_new_string };
    }


    auto skip_comments_and_whitespace(Lex_context& context) -> void {
        context.consume(is_space);

        auto const anchor = context.pointer;

        if (context.try_consume('/')) {
            switch (context.extract_current()) {
            case '/':
                context.consume(is_not_one_of<'\n'>);
                break;
            case '*':
                for (bu::Usize depth = 1; depth != 0; ) {
                    if (context.is_finished()) {
                        throw context.error(
                            anchor,
                            "Unterminating comment",
                            "Comments starting with '/*' can be terminated with '*/'"
                        );
                    }
                    else if (context.try_consume("*/")) {
                        --depth;
                    }
                    else if (context.try_consume("/*")) {
                        ++depth;
                    }
                    else {
                        ++context.pointer;
                    }
                }
                break;
            default:
                context.pointer -= 2;
                return;
            }

            skip_comments_and_whitespace(context);
        }
    }


    auto extract_identifier(Lex_context& context) -> bool {
        constexpr auto is_valid_head = satisfies_one_of<is_alpha, is_one_of<'_'>>;
        constexpr auto is_identifier = satisfies_one_of<is_alnum, is_one_of<'_', '\''>>;

        if (!is_valid_head(context.current())) {
            return false;
        }

        auto const view = context.extract(is_identifier);

        if (std::ranges::all_of(view, is_one_of<'_'>)) [[unlikely]] {
            return context.success({ .type = Type::underscore });
        }

        static auto const options = std::to_array<bu::Pair<lexer::Identifier, Type>>({
            { new_id("let")      , Type::let       },
            { new_id("mut")      , Type::mut       },
            { new_id("if")       , Type::if_       },
            { new_id("else")     , Type::else_     },
            { new_id("for")      , Type::for_      },
            { new_id("in")       , Type::in        },
            { new_id("while")    , Type::while_    },
            { new_id("loop")     , Type::loop      },
            { new_id("continue") , Type::continue_ },
            { new_id("break")    , Type::break_    },
            { new_id("match")    , Type::match     },
            { new_id("ret")      , Type::ret       },
            { new_id("fn")       , Type::fn        },
            { new_id("as")       , Type::as        },
            { new_id("data")     , Type::data      },
            { new_id("struct")   , Type::struct_   },
            { new_id("class")    , Type::class_    },
            { new_id("inst")     , Type::inst      },
            { new_id("alias")    , Type::alias     },
            { new_id("import")   , Type::import    },
            { new_id("module")   , Type::module    },
            { new_id("size_of")  , Type::size_of   },
            { new_id("type_of")  , Type::type_of   },
            { new_id("meta")     , Type::meta      },
        });

        static auto const
            true_id = new_id("true"),
            false_id = new_id("false");

        lexer::Identifier const identifier { view };

        if (identifier == true_id || identifier == false_id) {
            return context.success({ view.front() == 't', Type::boolean });
        }

        for (auto const [keyword, keyword_type] : options) {
            if (identifier == keyword) {
                return context.success({ .type = keyword_type });
            }
        }

        return context.success({
            identifier,
            is_upper(view.front())
                ? Type::upper_name
                : Type::lower_name
        });
    }

    auto extract_operator(Lex_context& context) -> bool {
        constexpr auto is_operator = is_one_of<
            '+', '-', '*', '/', '.', '|', '<', '=', '>', ':',
            '!', '?', '#', '%', '&', '^', '~', '$', '@', '\\'
        >;
        
        auto const view = context.extract(is_operator);
        if (view.empty()) {
            return false;
        }

        static constexpr auto clashing = std::to_array<bu::Pair<std::string_view, Type>>({
            { "." , Type::dot          },
            { ":" , Type::colon        },
            { "::", Type::double_colon },
            { "|" , Type::pipe         },
            { "=" , Type::equals       },
            { "&" , Type::ampersand    },
            { "->", Type::right_arrow  },
        });

        for (auto [punctuation, punctuation_type] : clashing) {
            if (view == punctuation) {
                return context.success({ .type = punctuation_type });
            }
        }

        return context.success({ lexer::Identifier { view }, Type::operator_name });
    }

    auto extract_punctuation(Lex_context& context) -> bool {
        static constexpr auto options = std::to_array<bu::Pair<char, Type>>({
            { ',', Type::comma         },
            { ';', Type::semicolon     },
            { '(', Type::paren_open    },
            { ')', Type::paren_close   },
            { '{', Type::brace_open    },
            { '}', Type::brace_close   },
            { '[', Type::bracket_open  },
            { ']', Type::bracket_close },
        });

        char const current = context.extract_current();

        for (auto const [character, punctuation_type] : options) {
            if (character == current) {
                return context.success({ .type = punctuation_type });
            }
        }

        if (current == ':') {
            return context.success({
                .type = context.try_consume(':')
                    ? Type::double_colon
                    : Type::colon
            });
        }

        --context.pointer;
        return false;
    }

    auto extract_numeric(Lex_context& context) -> bool {
        auto const old_state = context.state();

        bool negative = false;
        if (context.try_consume('-')) {
            negative = true;
        }

        int base = 10;
        if (context.try_consume('0')) {
            switch (context.extract_current()) {
            case 'b': base = 2; break;
            case 'q': base = 4; break;
            case 'o': base = 8; break;
            case 'd': base = 12; break;
            case 'x': base = 16; break;
            default:
                context.pointer -= 2;
            }

            if (base != 10 && context.try_consume('-')) {
                throw context.error(context.pointer - 1, "'-' must be applied before the base specifier");
            }
        }

        auto const anchor = context.pointer;

        bu::Isize integer;
        auto const [ptr, ec] = std::from_chars(anchor, context.stop, integer, base);

        if (anchor == ptr) {
            if (base != 10) {
                throw context.error(
                    { anchor - 2, anchor },
                    std::format("Expected an integer literal after the base-{} specifier", base)
                );
            }
            else {
                return context.failure(old_state);
            }
        }
        else if (*ptr == '.') {
            if (base != 10) {
                throw context.error({ anchor - 2, anchor }, "Float literals must be base-10");
            }

            bu::Float floating;
            auto const [ptr, ec] = std::from_chars(anchor, context.stop, floating);
            if (anchor == ptr) {
                bu::unimplemented();
            }
            else if (ec == std::errc::result_out_of_range) {
                throw context.error({ anchor, ptr }, "Float literal is too large");
            }
            else {
                assert(ec == std::errc {});
                context.pointer = ptr;
                return context.success({ negative ? -floating : floating, Type::floating });
            }
        }
        else {
            context.pointer = ptr;

            auto const report_too_large = [&](char const* ptr) {
                throw context.error({ old_state.pointer, ptr }, "integer literal is too large");
            };

            if (ec == std::errc::result_out_of_range) {
                report_too_large(ptr);
            }
            else if (integer < 0 && negative) [[unlikely]] {
                throw context.error(old_state.pointer + 1, "only one '-' may be applied");
            }

            if (context.try_consume('e') || context.try_consume('E')) {
                bu::I8 exponent;
                auto [ptr, ec] = std::from_chars(context.pointer, context.stop, exponent);

                if (context.pointer == ptr) {
                    throw context.error(ptr, "expected an exponent");
                }
                else if (exponent < 0) {
                    throw context.error(
                        context.pointer,
                        "negative exponent",
                        "use a floating point literal if this was intended"
                    );
                }
                else if (ec == std::errc::result_out_of_range ||
                    (bu::digit_count(integer) + exponent) >= std::numeric_limits<bu::Isize>::digits10)
                {
                    report_too_large(ptr);
                }

                bu::Isize coefficient = 1;
                while (exponent--) {
                    coefficient *= 10;
                }

                integer *= coefficient;
                context.pointer = ptr;
            }

            assert(ec == std::errc {});
            return context.success({ negative ? -integer : integer, Type::integer });
        }
    }


    auto handle_escape_sequence(Lex_context& context) -> char {
        auto const anchor = context.pointer;

        switch (context.extract_current()) {
        case 'a': return '\a';
        case 'b': return '\b';
        case 'f': return '\f';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case 'v': return '\v';
        case '\'': return '\'';
        case '\"': return '\"';
        case '\\': return '\\';
        case '\0':
            throw context.error(anchor, "Expected an escape sequence, but found the end of input");
        default:
            throw context.error(anchor, "Unrecognized escape sequence");
        }
    }


    auto extract_character(Lex_context& context) -> bool {
        auto const anchor = context.pointer;

        if (context.try_consume('\'')) {
            char c = context.extract_current();

            if (c == '\0') {
                throw context.error(anchor, "Unterminating character literal");
            }
            else if (c == '\\') {
                c = handle_escape_sequence(context);
            }

            if (context.try_consume('\'')) {
                return context.success({ c, Type::character });
            }
            else {
                throw context.error(context.pointer, "Expected a closing single-quote");
            }
        }
        else {
            return false;
        }
    }

    auto extract_string(Lex_context& context) -> bool {
        auto const anchor = context.pointer;

        if (context.try_consume('"')) {
            std::string string;

            for (;;) {
                switch (char c = context.extract_current()) {
                case '\0':
                    throw context.error(anchor, "Unterminating string literal");
                case '"':
                    return context.success({ lexer::String { std::move(string) }, Type::string });
                case '\\':
                    c = handle_escape_sequence(context);
                    [[fallthrough]];
                default:
                    string.push_back(c);
                }
            }
        }
        else {
            return false;
        }
    }

}


auto lexer::lex(bu::Source&& source) -> Tokenized_source {
    Lex_context context { source };

    constexpr std::array extractors {
        extract_identifier,
        extract_numeric,
        extract_operator,
        extract_punctuation,
        extract_string,
        extract_character,
    };

    for (;;) {
        skip_comments_and_whitespace(context);
        context.token_start = context.pointer;

        bool did_extract = false;

        for (auto extractor : extractors) {
            if (extractor(context)) {
                did_extract = true;
                break;
            }
        }

        if (!did_extract) {
            if (context.is_finished()) {
                context.tokens.push_back({ .type = Type::end_of_input, .source_view = context.pointer });
                return { std::move(source), std::move(context.tokens) };
            }
            else {
                throw context.error(context.pointer, "Syntax error; unable to extract lexical token");
            }
        }
    }
}


auto lexer::literals::operator"" _id(char const* string, bu::Usize length) noexcept -> Identifier {
    return Identifier { std::string_view { string, length } };
}