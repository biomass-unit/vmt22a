#include "bu/utilities.hpp"
#include "lexer/lexer.hpp"
#include "tests/tests.hpp"


namespace {

    auto assert_tok_eq(std::string_view const          text,
                       std::vector<lexer::Token::Type> required_types,
                       std::source_location            caller = std::source_location::current())
        -> void
    {
        bu::Source source { bu::Source::Mock_tag {}, std::string { text } };
        auto tokens = lexer::lex(std::move(source)).tokens;

        required_types.push_back(lexer::Token::Type::end_of_input);

        std::vector<lexer::Token::Type> actual_types;
        actual_types.reserve(required_types.size());

        std::ranges::move(
            tokens | std::views::transform(&lexer::Token::type),
            std::back_inserter(actual_types)
        );

        tests::assert_eq(required_types, actual_types, caller);
    }


    auto run_lexer_tests() -> void {
        using namespace tests;
        using enum lexer::Token::Type;

        "whitespace"_test = [] {
            assert_tok_eq(
                "\ta\nb  \t  c  \n  d\n\n e ",
                { lower_name, lower_name, lower_name, lower_name, lower_name }
            );
        };

        "numeric"_test = [] {
            assert_tok_eq(
                "50 23.4 0xdeadbeef 1. -3",
                { integer, floating, integer, floating, integer }
            );

            assert_tok_eq(
                "0.3e-5 3e3 -0. -0.2E5",
                { floating, integer, floating, floating }
            );
        };

        "tuple_member_access"_test = [] {
            assert_tok_eq(
                ".0.0, 0.0",
                { dot, integer, dot, integer, comma, floating }
            );
        };

        "punctuation"_test = [] {
            assert_tok_eq(
                "\n::\t,;(--? @#",
                { double_colon, comma, semicolon, paren_open, operator_name, operator_name }
            );
        };

        "comment"_test = [] {
            assert_tok_eq(
                ". /* , /*::*/! */ in /**/ / //",
                { dot, in, operator_name }
            );

            assert_tok_eq(
                "/* \"\" */ . /* \"*/\" */ . \"/* /*\" . /* /* \"*/\"*/ */ .",
                { dot, dot, string, dot, dot }
            );
        };

        "keyword"_test = [] {
            assert_tok_eq(
                "for;forr(for2",
                { for_, semicolon, lower_name, paren_open, lower_name }
            );

            assert_tok_eq(
                ",.[}\tmatch::",
                { comma, dot, bracket_open, brace_close, match, double_colon }
            );
        };

        "pattern"_test = [] {
            assert_tok_eq(
                "x1 _ wasd,3",
                { lower_name, underscore, lower_name, comma, integer }
            );

            assert_tok_eq(
                "a<$>_:\nVec",
                { lower_name, operator_name, underscore, colon, upper_name }
            );

            assert_tok_eq(
                "_, ______::_________________",
                { underscore, comma, underscore, double_colon, underscore }
            );
        };

        "string"_test = [] {
            assert_tok_eq(
                "\"test\\t\\\",\", 'a', '\\\\'",
                { string, comma, character, comma, character }
            );

            assert_tok_eq(
                "\"hmm\" \", yes\"",
                { string }
            );
        };

        "casing"_test = [] {
            assert_tok_eq(
                "a A _a _A _0 _",
                { lower_name, upper_name, lower_name, upper_name, lower_name, underscore }
            );
        };
    }

}


REGISTER_TEST(run_lexer_tests);