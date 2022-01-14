#include "bu/utilities.hpp"
#include "lexer.hpp"
#include "token_formatting.hpp"

#include <charconv>


namespace {

    using Type = lexer::Token::Type;

    class Lex_context {
        std::vector<lexer::Token>* tokens;
    public:
        char const* start;
        char const* stop;
        char const* pointer;
        bu::Usize line   = 1;
        bu::Usize column = 1;

        explicit Lex_context(std::string_view const source, std::vector<lexer::Token>& tokens) noexcept
            : tokens  { &tokens               }
            , start   { source.data()         }
            , stop    { start + source.size() }
            , pointer { start                 }
        {
            tokens.reserve(1024);
        }

        inline auto is_finished() const noexcept -> bool {
            return pointer == stop;
        }

        inline auto current() const noexcept -> char {
            return *pointer;
        }

        inline auto extract_current() noexcept -> char {
            update_location();
            return *pointer++;
        }

        inline auto consume(std::predicate<char> auto const predicate) noexcept -> void {
            for (; (pointer != stop) && predicate(*pointer); ++pointer) {
                update_location();
            }
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
                ++column;
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
            column += string.size();
            return true;
        }

        inline auto success(lexer::Token&& token) noexcept -> std::true_type {
            tokens->push_back(std::move(token));
            return {};
        }
    private:
        inline auto update_location() noexcept -> void {
            if (*pointer == '\n') {
                ++line;
                column = 1;
            }
            else {
                ++column;
            }
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

        if (context.try_consume('/')) {
            switch (context.extract_current()) {
            case '/':
                context.consume(is_not_one_of<'\n'>);
                break;
            case '*':
                for (bu::Usize depth = 1; depth != 0; ) {
                    if (context.is_finished()) {
                        bu::abort("unterminating comment");
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
            { new_id("size_of")  , Type::size_of   },
            { new_id("type_of")  , Type::type_of   },
            { new_id("meta")     , Type::meta      },
            { new_id("mod")      , Type::mod       },
        });

        lexer::Identifier const identifier { view };

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
        }

        auto const anchor = context.pointer;

        bu::Isize integer;
        auto const [ptr, ec] = std::from_chars(anchor, context.stop, integer, base);

        if (anchor == ptr) {
            if (base != 10) {
                bu::abort(std::format("expected an integer literal after the base-{} specifier", base));
            }
            else {
                return false;
            }
        }
        else if (*ptr == '.') {
            if (base != 10) {
                bu::abort("float literals must be base-10");
            }

            bu::Float floating;
            auto const [ptr, ec] = std::from_chars(anchor, context.stop, floating);
            if (anchor == ptr) {
                bu::unimplemented();
            }
            else if (ec == std::errc::result_out_of_range) {
                bu::abort("float literal too large");
            }
            else {
                assert(ec == std::errc {});
                context.pointer = ptr;
                return context.success({ floating, Type::floating });
            }
        }
        else if (ec == std::errc {}) {
            context.pointer = ptr;
            return context.success({ integer, Type::integer });
        }
        else {
            bu::unimplemented();
        }
    }

}


auto lexer::lex(std::string_view const source) -> std::vector<Token> {
    std::vector<Token> tokens;
    Lex_context context { source, tokens };

    constexpr std::array extractors {
        extract_identifier,
        extract_operator,
        extract_punctuation,
        extract_numeric,
    };

    for (;;) {
        skip_comments_and_whitespace(context);

        bool did_extract = false;

        for (auto extractor : extractors) {
            if (extractor(context)) {
                did_extract = true;
                break;
            }
        }

        if (!did_extract) {
            if (context.is_finished()) {
                tokens.push_back({ .type = Type::end_of_input });
                return tokens;
            }
            else {
                bu::abort("syntax error");
            }
        }
    }
}


auto lexer::literals::operator"" _id(char const* string, bu::Usize length) noexcept -> Identifier {
    return Identifier { std::string_view { string, length } };
}