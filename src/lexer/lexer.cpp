#include "bu/utilities.hpp"
#include "bu/textual_error.hpp"
#include "lexer.hpp"
#include "token_formatting.hpp"

#include <charconv>


namespace {

    using lexer::Token;

    struct Lex_context {
        using Error = bu::Exception;

        std::vector<Token> tokens;
        bu::Source*        source;
        char const*        start;
        char const*        stop;

        struct State {
            char const* pointer = nullptr;
            bu::Usize   line    = 1;
            bu::Usize   column  = 1;
        };

        State token_start;

    private:

        State state;

        auto update_location(char const c) noexcept -> void {
            if (c == '\n') {
                ++state.line;
                state.column = 1;
            }
            else {
                ++state.column;
            }
        }

        [[nodiscard]]
        auto get_source_position(std::string_view const view) const
            noexcept -> bu::Pair<bu::Source_position>
        {
            bu::Source_position start_pos;

            for (char const* ptr = start; ptr != view.data(); ++ptr) {
                start_pos.increment_with(*ptr);
            }

            bu::Source_position stop_pos = start_pos;

            for (char const c : view) {
                stop_pos.increment_with(c);
            }

            return { start_pos, stop_pos };
        }

    public:

        explicit Lex_context(bu::Source& source) noexcept
            : source      { &source }
            , start       { source.string().data() }
            , stop        { start + source.string().size() }
            , token_start { .pointer = start }
            , state       { .pointer = start }
        {
            tokens.reserve(1024);
        }


        auto current_state() const noexcept -> State {
            return state;
        }

        auto restore(State const old) noexcept -> void {
            state = old;
        }

        auto is_finished() const noexcept -> bool {
            return state.pointer == stop;
        }

        auto advance(bu::Usize const distance = 1) noexcept -> void {
            for (bu::Usize i = 0; i != distance; ++i) {
                update_location(*state.pointer++);
            }
        }

        auto current_pointer() const noexcept -> char const* {
            return state.pointer;
        }

        auto current() const noexcept -> char {
            return *state.pointer;
        }

        auto extract_current() noexcept -> char {
            update_location(*state.pointer);
            return *state.pointer++;
        }

        auto consume(std::predicate<char> auto const predicate) noexcept -> void {
            for (; (state.pointer != stop) && predicate(*state.pointer); ++state.pointer) {
                update_location(*state.pointer);
            }
        }

        auto extract(std::predicate<char> auto const predicate) noexcept -> std::string_view {
            auto* const anchor = state.pointer;
            consume(predicate);
            return { anchor, state.pointer };
        }

        auto try_consume(char const c) noexcept -> bool {
            assert(c != '\n');

            if (*state.pointer == c) {
                ++state.column;
                ++state.pointer;
                return true;
            }
            else {
                return false;
            }
        }

        auto try_consume(std::string_view const string) noexcept -> bool {
            char const* ptr = state.pointer;

            for (char const character : string) {
                assert(character != '\n');
                if (*ptr++ != character) {
                    return false;
                }
            }

            state.pointer = ptr;
            state.column += string.size();

            return true;
        }

        auto success(Token::Type const type, Token::Variant&& value = std::monostate {})
            noexcept -> std::true_type
        {
            tokens.emplace_back(
                std::move(value),
                type,
                bu::Source_view {
                    std::string_view    { token_start.pointer, state      .pointer },
                    bu::Source_position { token_start.line   , token_start.column  },
                    bu::Source_position { state      .line   ,       state.column  }
                }
            );

            return {};
        }

        auto error(
            std::string_view                const view,
            std::string_view                const message,
            std::optional<std::string_view> const help = std::nullopt
        )
            const -> Error
        {
            auto const [start_pos, stop_pos] = get_source_position(view);

            return Error {
                bu::simple_textual_error({
                    .erroneous_view = bu::Source_view { view, start_pos, stop_pos },
                    .source         = source,
                    .message        = message,
                    .help_note      = help
                })
            };
        }

        auto error(
            char const*                     const location,
            std::string_view                const message,
            std::optional<std::string_view> const help = std::nullopt
        )
            const -> Error
        {
            return error({ location, location + 1 }, message, help);
        }

        template <bu::trivial T>
        struct Parse_result {
            std::from_chars_result result;
            char const* start_position;
            T private_value;

            auto get() const noexcept -> T {
                assert(result.ec == std::errc {});
                return private_value;
            }

            auto did_parse() const noexcept -> bool {
                return result.ptr != start_position;
            }

            auto is_too_large() const noexcept -> bool {
                return result.ec == std::errc::result_out_of_range;
            }

            auto was_non_numeric() const noexcept -> bool {
                return result.ec == std::errc::invalid_argument;
            }
        };

        template <bu::trivial T>
        auto parse(std::same_as<int> auto const... args) noexcept -> Parse_result<T> {
            static_assert(
                sizeof...(args) <= 1,
                "The parameter pack is used for the optional base parameter only"
            );

            T value;
            auto const result = std::from_chars(state.pointer, stop, value, args...);
            return { result, state.pointer, value };
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
        return lexer::Identifier { id, bu::Pooled_string_strategy::guaranteed_new_string };
    }


    auto skip_comments_and_whitespace(Lex_context& context) -> void {
        context.consume(is_space);

        auto const state = context.current_state();

        if (context.try_consume('/')) {
            switch (context.extract_current()) {
            case '/':
                context.consume(is_not_one_of<'\n'>);
                break;
            case '*':
                for (bu::Usize depth = 1; depth != 0; ) {
                    if (context.is_finished()) {
                        throw context.error(
                            state.pointer,
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
                        context.advance();
                    }
                }
                break;
            default:
                context.restore(state);
                return;
            }

            skip_comments_and_whitespace(context);
        }
    }


    auto extract_identifier(Lex_context& context) -> bool {
        static constexpr auto is_valid_head = satisfies_one_of<is_alpha, is_one_of<'_'>>;
        static constexpr auto is_identifier = satisfies_one_of<is_alnum, is_one_of<'_', '\''>>;

        if (!is_valid_head(context.current())) {
            return false;
        }

        auto const view = context.extract(is_identifier);

        if (std::ranges::all_of(view, is_one_of<'_'>)) [[unlikely]] {
            return context.success(Token::Type::underscore);
        }

        static auto const options = std::to_array<bu::Pair<lexer::Identifier, Token::Type>>({
            { new_id("let")       , Token::Type::let            },
            { new_id("mut")       , Token::Type::mut            },
            { new_id("if")        , Token::Type::if_            },
            { new_id("else")      , Token::Type::else_          },
            { new_id("elif")      , Token::Type::elif           },
            { new_id("for")       , Token::Type::for_           },
            { new_id("in")        , Token::Type::in             },
            { new_id("while")     , Token::Type::while_         },
            { new_id("loop")      , Token::Type::loop           },
            { new_id("continue")  , Token::Type::continue_      },
            { new_id("break")     , Token::Type::break_         },
            { new_id("match")     , Token::Type::match          },
            { new_id("ret")       , Token::Type::ret            },
            { new_id("fn")        , Token::Type::fn             },
            { new_id("as")        , Token::Type::as             },
            { new_id("String")    , Token::Type::string_type    },
            { new_id("Int")       , Token::Type::integer_type   },
            { new_id("Float")     , Token::Type::floating_type  },
            { new_id("Char")      , Token::Type::character_type },
            { new_id("Bool")      , Token::Type::boolean_type   },
            { new_id("enum")      , Token::Type::enum_          },
            { new_id("struct")    , Token::Type::struct_        },
            { new_id("class")     , Token::Type::class_         },
            { new_id("inst")      , Token::Type::inst           },
            { new_id("impl")      , Token::Type::impl           },
            { new_id("alias")     , Token::Type::alias          },
            { new_id("namespace") , Token::Type::namespace_     },
            { new_id("import")    , Token::Type::import_        },
            { new_id("export")    , Token::Type::export_        },
            { new_id("module")    , Token::Type::module_        },
            { new_id("size_of")   , Token::Type::size_of        },
            { new_id("type_of")   , Token::Type::type_of        },
            { new_id("meta")      , Token::Type::meta           },
            { new_id("where")     , Token::Type::where          },
            { new_id("immut")     , Token::Type::immut          },
            { new_id("dyn")       , Token::Type::dyn            },
            { new_id("pub")       , Token::Type::pub            },
        });

        static lexer::Identifier const
            true_id  = new_id("true" ),
            false_id = new_id("false");

        lexer::Identifier const identifier { view };

        if (identifier == true_id || identifier == false_id) {
            return context.success(Token::Type::boolean, view.front() == 't');
        }

        for (auto const [keyword, keyword_type] : options) {
            if (identifier == keyword) {
                return context.success(keyword_type);
            }
        }

        return context.success(
            is_upper(view.front())
                ? Token::Type::upper_name
                : Token::Type::lower_name,
            identifier
        );
    }

    auto extract_operator(Lex_context& context) -> bool {
        static constexpr auto is_operator = is_one_of<
            '+', '-', '*', '/', '.', '|', '<', '=', '>', ':',
            '!', '?', '#', '%', '&', '^', '~', '$', '@', '\\'
        >;
        
        auto const view = context.extract(is_operator);
        if (view.empty()) {
            return false;
        }

        static constexpr auto clashing = std::to_array<bu::Pair<std::string_view, Token::Type>>({
            { "."  , Token::Type::dot          },
            { ":"  , Token::Type::colon        },
            { "::" , Token::Type::double_colon },
            { "|"  , Token::Type::pipe         },
            { "="  , Token::Type::equals       },
            { "&"  , Token::Type::ampersand    },
            { "*"  , Token::Type::asterisk     },
            { "+"  , Token::Type::plus         },
            { "?"  , Token::Type::question     },
            { "\\" , Token::Type::lambda       },
            { "->" , Token::Type::right_arrow  },
            { "???", Token::Type::hole         },
        });

        for (auto [punctuation, punctuation_type] : clashing) {
            if (view == punctuation) {
                return context.success(punctuation_type);
            }
        }

        return context.success(Token::Type::operator_name, lexer::Identifier { view });
    }

    auto extract_punctuation(Lex_context& context) -> bool {
        static constexpr auto options = std::to_array<bu::Pair<char, Token::Type>>({
            { ',', Token::Type::comma         },
            { ';', Token::Type::semicolon     },
            { '(', Token::Type::paren_open    },
            { ')', Token::Type::paren_close   },
            { '{', Token::Type::brace_open    },
            { '}', Token::Type::brace_close   },
            { '[', Token::Type::bracket_open  },
            { ']', Token::Type::bracket_close },
        });

        char const current = context.current();

        for (auto const [character, punctuation_type] : options) {
            if (character == current) {
                context.advance();
                return context.success(punctuation_type);
            }
        }

        return false;
    }


    auto extract_numeric_base(Lex_context& context) -> int {
        int base = 10;
        
        auto const state = context.current_state();

        if (context.try_consume('0')) {
            switch (context.extract_current()) {
            case 'b': base = 2; break;
            case 'q': base = 4; break;
            case 'o': base = 8; break;
            case 'd': base = 12; break;
            case 'x': base = 16; break;
            default:
                context.restore(state);
                return base;
            }

            if (context.try_consume('-')) {
                throw context.error(
                    state.pointer - 1,
                    "'-' must be applied before the base specifier"
                );
            }
        }

        return base;
    }

    auto apply_scientific_coefficient(bu::Isize& integer, char const* anchor, Lex_context& context) -> void {
        if (context.try_consume('e') || context.try_consume('E')) {
            auto const exponent = context.parse<bu::Isize>();

            if (exponent.did_parse()) {
                if (exponent.is_too_large()) {
                    throw context.error(
                        { exponent.start_position, exponent.result.ptr },
                        "Exponent is too large"
                    );
                }
                if (exponent.get() < 0) [[unlikely]] {
                    throw context.error(
                        context.current_pointer(),
                        "Negative exponent",
                        "use a floating point literal if this was intended"
                    );
                }
                else if (bu::digit_count(integer) + exponent.get() >= std::numeric_limits<bu::Isize>::digits10) {
                    throw context.error(
                        { anchor, exponent.result.ptr },
                        "Integer literal is too large after applying scientific coefficient"
                    );
                }

                auto value = exponent.get();
                bu::Isize coefficient = 1;
                while (value--) {
                    coefficient *= 10;
                }

                integer *= coefficient;
                context.advance(
                    bu::unsigned_distance(
                        context.current_pointer(),
                        exponent.result.ptr
                    )
                );
            }
            else if (exponent.was_non_numeric()) {
                throw context.error(
                    exponent.start_position,
                    "Expected an exponent"
                );
            }
            else {
                bu::todo();
            }
        }
    }

    auto extract_numeric(Lex_context& context) -> bool {
        auto const state = context.current_state();

        bool const negative = context.try_consume('-');
        auto const base     = extract_numeric_base(context);
        auto const integer  = context.parse<bu::Isize>(base);

        if (integer.was_non_numeric()) {
            if (base == 10) {
                context.restore(state);
                return false;
            }
            else {
                throw context.error(
                    { state.pointer, 2 }, // view of the base specifier
                    "Expected an integer literal after the base-{} specifier"_format(base)
                );
            }
        }
        else if (integer.is_too_large()) {
            throw context.error(
                { state.pointer, integer.result.ptr },
                "Integer literal is too large"
            );
        }
        else if (!integer.did_parse()) {
            bu::todo();
        }

        if (negative && integer.get() < 0) {
            throw context.error(state.pointer + 1, "Only one '-' may be applied");
        }

        auto const is_tuple_member_index = [&] {
            // If the numeric literal is preceded by '.', then don't attempt to
            // parse a float. This allows nested tuple member-access: tuple.0.0
            return state.pointer != context.start
                ? state.pointer[-1] == '.'
                : false;
        };

        if (*integer.result.ptr == '.' && !is_tuple_member_index()) {
            if (base != 10) {
                throw context.error({ state.pointer, 2 }, "Float literals must be base-10");
            }

            context.restore(state);
            auto const floating = context.parse<bu::Float>();

            if (floating.did_parse()) {
                if (floating.is_too_large()) {
                    throw context.error(
                        { state.pointer, floating.result.ptr },
                        "Floating-point literal is too large"
                    );
                }
                else {
                    context.advance(
                        bu::unsigned_distance(
                            context.current_pointer(),
                            floating.result.ptr
                        )
                    );
                    return context.success(Token::Type::floating, floating.get());
                }
            }
            else {
                bu::todo();
            }
        }

        auto value = integer.get();
        value *= (negative ? -1 : 1);

        context.advance(
            bu::unsigned_distance(
                context.current_pointer(),
                integer.result.ptr
            )
        );
        apply_scientific_coefficient(value, state.pointer, context);

        return context.success(Token::Type::integer, value);
    }


    auto handle_escape_sequence(Lex_context& context) -> char {
        auto* const anchor = context.current_pointer();

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
        auto* const anchor = context.current_pointer();

        if (context.try_consume('\'')) {
            char c = context.extract_current();

            if (c == '\0') {
                throw context.error(anchor, "Unterminating character literal");
            }
            else if (c == '\\') {
                c = handle_escape_sequence(context);
            }

            if (context.try_consume('\'')) {
                return context.success(Token::Type::character, c);
            }
            else {
                throw context.error(context.current_pointer(), "Expected a closing single-quote");
            }
        }
        else {
            return false;
        }
    }

    auto extract_string(Lex_context& context) -> bool {
        auto* const anchor = context.current_pointer();

        if (context.try_consume('"')) {
            std::string string;

            for (;;) {
                switch (char c = context.extract_current()) {
                case '\0':
                    throw context.error(anchor, "Unterminating string literal");
                case '"':
                    if (!context.tokens.empty() && context.tokens.back().type == Token::Type::string) {
                        // Concatenate adjacent string literals

                        string.insert(0, context.tokens.back().as_string().view());
                        context.tokens.pop_back(); // Pop the previous string
                    }
                    return context.success(Token::Type::string, lexer::String { std::move(string) });
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

    if (source.string().empty()) {
        return { .source = std::move(source) };
    }

    static constexpr std::array extractors {
        extract_identifier,
        extract_numeric,
        extract_operator,
        extract_punctuation,
        extract_string,
        extract_character,
    };

    for (;;) {
        skip_comments_and_whitespace(context);
        context.token_start = context.current_state();

        bool did_extract = false;

        for (auto extractor : extractors) {
            if (extractor(context)) {
                did_extract = true;
                break;
            }
        }

        if (!did_extract) {
            if (context.is_finished()) {
                auto const state = context.current_state();

                context.tokens.push_back(
                    Token {
                        .value       = std::monostate {},
                        .type        = Token::Type::end_of_input,
                        .source_view = bu::Source_view {
                            std::string_view { context.stop, context.stop },
                            { state.line, state.column },
                            { state.line, state.column }
                        }
                    }
                );

                return { std::move(source), std::move(context.tokens) };
            }
            else {
                throw context.error(
                    context.current_pointer(),
                    "Syntax error; unable to extract lexical token"
                );
            }
        }
    }
}


auto lexer::literals::operator"" _id(char const* string, bu::Usize length) noexcept -> Identifier {
    return Identifier { std::string_view { string, length } };
}