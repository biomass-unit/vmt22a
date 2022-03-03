#include "bu/utilities.hpp"
#include "bu/color.hpp"

#include "lexer/lexer.hpp"
#include "lexer/token_formatting.hpp"

#include "ast/ast.hpp"
#include "ast/ast_formatting.hpp"

#include "parser/parser.hpp"
#include "parser/internals/parser_internals.hpp" // for the repl only

#include "vm/bytecode.hpp"
#include "vm/virtual_machine.hpp"
#include "vm/vm_formatting.hpp"

#include "compiler/codegen_internals.hpp"

#include "tests/tests.hpp"


namespace {

    auto generic_repl(std::invocable<bu::Source> auto f) {
        return [=] {
            for (;;) {
                auto string = bu::string_without_sso();

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
    auto const lexer_repl = generic_repl([](bu::Source source) {
        bu::print("Tokens: {}\n", lexer::lex(std::move(source)).tokens);
    });

    [[maybe_unused]]
    auto const expression_parser_repl = generic_repl([](bu::Source source) {
        auto tokenized_source = lexer::lex(std::move(source));
        parser::Parse_context context { tokenized_source };

        auto result = parser::parse_expression(context);
        bu::print("Result: {}\nRemaining input: '{}'\n", result, context.pointer->source_view.data());
    });

    [[maybe_unused]]
    auto const program_parser_repl = generic_repl([](bu::Source source) {
        bu::print("{}\n", parser::parse(lexer::lex(std::move(source))));
    });

}


using namespace bu    :: literals;
using namespace lexer :: literals;


auto main() -> int try {
    bu::enable_color_formatting ();
    tests::run_all_tests        ();
    //program_parser_repl         ();
    //expression_parser_repl      ();

    auto module = parser::parse(
        lexer::lex(
            bu::Source {
                bu::Source::Mock_tag {},
R"(

    module std {
        struct Test = a: Int, b: (Int, Int)
    }


    alias A = std::Test
    alias B = A
    alias C = B

    data T = a(A, B) | b(C)

)"
            }
        )
    );

    auto tokens = lexer::lex(bu::Source { bu::Source::Mock_tag {}, "T" });
    parser::Parse_context parse_context { tokens };
    auto type = parser::extract_type(parse_context);

    compiler::Codegen_context context { { .stack = bu::Bytestack { 1000 } }, std::move(module) };

    bu::print("size: {}\n", compiler::size_of(type, context));
}

catch (std::exception const& exception) {
    std::cerr << bu::Color::red << "Error: " << bu::Color::white << exception.what() << '\n';
}

// fix scoping issue