#include "bu/utilities.hpp"
#include "bu/color.hpp"

#include "lexer/lexer.hpp"
#include "lexer/lexer_test.hpp"
#include "lexer/token_formatting.hpp"

#include "ast/ast.hpp"
#include "ast/ast_formatting.hpp"

#include "parser/parser.hpp"
#include "parser/parser_test.hpp"
#include "parser/internals/parser_internals.hpp" // for the repl only

#include "vm/bytecode.hpp"
#include "vm/virtual_machine.hpp"
#include "vm/vm_formatting.hpp"
#include "vm/vm_test.hpp"


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
    auto lexer_repl = generic_repl([](bu::Source source) {
        bu::print("Tokens: {}\n", lexer::lex(std::move(source)).tokens);
    });

    [[maybe_unused]]
    auto parser_repl = generic_repl([](bu::Source source) {
        auto tokenized_source = lexer::lex(std::move(source));
        parser::Parse_context context { tokenized_source };

        auto result = parser::parse_expression(context);
        bu::print("Result: {}\nRemaining input: '{}'\n", result, context.pointer->source_view.data());
    });

    [[maybe_unused]]
    auto program_parser_repl = generic_repl([](bu::Source source) {
        ast::AST_context context { 32 };
        auto module = parser::parse(lexer::lex(std::move(source)));
        bu::print("{}\n", module.global_namespace);
    });

}


using namespace bu::literals;
using namespace lexer::literals;


auto main() -> int try {
    bu::enable_color_formatting();

    lexer  :: run_tests();
    parser :: run_tests();
    vm     :: run_tests();

    program_parser_repl();

    /*vm::Virtual_machine machine { .stack = bu::Bytestack { 1000 } };

    using enum vm::Opcode;
    machine.bytecode.write(
        ipush, 5_iz,
        iprint,
        jump, 0_uz
    );

    return machine.run();*/
}

catch (std::bad_alloc const&) {
    std::cerr << bu::Color::red << "Error:" << bu::Color::white << " bad allocation\n";
}

catch (std::exception const& exception) {
    bu::print<std::cerr>("{}Error:{} {}\n", bu::Color::red, bu::Color::white, exception.what());
}