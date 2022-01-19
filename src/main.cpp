#include "bu/utilities.hpp"

#include "lexer/lexer.hpp"
#include "lexer/lexer_test.hpp"
#include "lexer/token_formatting.hpp"

#include "ast/ast.hpp"
#include "ast/ast_formatting.hpp"

#include "parser/parser.hpp"
#include "parser/parser_test.hpp"
#include "parser/internals/parser_internals.hpp" // for the repl only


namespace {

    auto generic_repl(std::invocable<bu::Source> auto f) {
        return [=] {
            for (;;) {
                std::string string;
                string.reserve(sizeof string); // disable SSO

                bu::print(" >>> ");
                std::getline(std::cin, string);

                if (string.empty()) {
                    continue;
                }

                try {
                    bu::Source source { bu::Source::Mock_tag {}, std::move(string) };
                    f(std::move(source));
                }
                catch (std::exception const& exception) {
                    bu::print<std::cerr>("REPL error: {}\n", exception.what());
                }
            }
        };
    }

    [[maybe_unused]]
    auto lexer_repl = generic_repl([](bu::Source src) {
        bu::print("Tokens: {}\n", lexer::lex(std::move(src)).tokens);
    });

    [[maybe_unused]]
    auto parser_repl = generic_repl([](bu::Source src) {
        auto [source, tokens] = lexer::lex(std::move(src));

        parser::Parse_context context {
            .pointer = tokens.data(),
            .source  = &source
        };

        auto result = parser::parse_expression(context);
        bu::print("Result: {}\nRemaining input: '{}'\n", result, context.pointer->source_view.data());
    });

}


auto main() -> int try {
    lexer::run_tests();
    parser_repl();
}

catch (std::bad_alloc const&) {
    std::cerr << "Error: bad allocation\n";
}

catch (std::exception const& exception) {
    bu::print<std::cerr>("Error: {}\n", exception.what());
}