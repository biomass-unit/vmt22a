#include "bu/utilities.hpp"
#include "lexer.hpp"

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
                if (*ptr++ != character) {
                    return false;
                }
            }

            pointer = ptr;
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


    /*auto extract_identifier(Lex_context&) -> bool {
        bu::unimplemented();
    }*/

    /*auto extract_punctuation(Lex_context&) -> bool {
        bu::unimplemented();
    }*/

    auto extract_numeric(Lex_context& context) -> bool {
        auto const anchor = context.pointer;

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

        bu::Isize integer;
        auto const [ptr, ec] = std::from_chars(context.pointer, context.stop, integer, base);

        if (anchor == ptr) {
            return false;
        }
        else if (ec == std::errc {}) {
            context.pointer = ptr;
            return context.success({ .value = integer, .type = Type::integer });
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
        //extract_identifier,
        //extract_punctuation,
        extract_numeric,
    };

    for (;;) {
        bool did_extract = false;

        for (auto extractor : extractors) {
            if (extractor(context)) {
                did_extract = true;
                break;
            }
        }

        if (!did_extract) {
            if (context.is_finished()) {
                return tokens;
            }
            else {
                bu::abort("syntax error");
            }
        }
    }
}