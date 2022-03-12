#include "bu/utilities.hpp"
#include "bu/color.hpp"
#include "bu/timer.hpp"

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

#include "cli/cli.hpp"


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
                    bu::print<std::cerr>("REPL error: {}\n\n", exception.what());
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


    template <auto extract>
    auto debug_parse(std::string_view string) {
        auto input = bu::string_without_sso();
        input = string;

        auto tokenized_source = lexer::lex(
            bu::Source {
                bu::Source::Mock_tag {},
                std::move(input)
            }
        );
        parser::Parse_context context { tokenized_source };

        return extract(context);
    }

}


using namespace bu    :: literals;
using namespace lexer :: literals;


auto main(int argc, char const** argv) -> int try {
    bu::enable_color_formatting();

    cli::Options_description description;
    description.add_options()
        ("help", "Show this text")
        ("repl", cli::string(), "Run the given repl");

    auto options = cli::parse_command_line(argc, argv, description);

    if (options.find("help")) {
        bu::print("Valid options:\n\n{}", description);
        return 0;
    }

    tests::run_all_tests();
    
    if (auto* const name = options.find_string("repl")) {
        if (*name == "lex") {
            lexer_repl();
        }
        else if (*name == "expr") {
            expression_parser_repl();
        }
        else if (*name == "prog") {
            program_parser_repl();
        }
        else {
            bu::abort("Unrecognized repl name");
        }
    }

    /*vm::Virtual_machine machine { .stack = bu::Bytestack { 1000 } };

    constexpr auto max = 5'000'000_iz;

    using enum vm::Opcode;
    machine.bytecode.write(
        ipush, 0_iz,
        iinc_top,
        idup,
        local_jump_ineq_i, vm::Local_offset_type(-13), max,
        halt
    );

    return machine.run();*/

    /*auto module = parser::parse(
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

    // data U = a(A, B) | b(Int)
    // alias T = type_of(b(3))

    fn g(n: Int): (Int, Int) = n
    alias G = type_of(g(5))

)"
            }
        )
    );

    auto type = debug_parse<parser::extract_type>("G");

    compiler::Codegen_context context { { .stack = bu::Bytestack { 1000 } }, std::move(module) };

    bu::print("type: {}\n", compiler::size_of(type, context));*/
}

catch (cli::Unrecognized_option const& exception) {
    std::cerr << exception.what() << "; use --help to see a list of valid options\n";
}

catch (std::exception const& exception) {
    std::cerr << bu::Color::red << "Error: " << bu::Color::white << exception.what() << '\n';
}