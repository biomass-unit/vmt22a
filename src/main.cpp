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

#include "ir/ir_formatting.hpp"
#include "resolution/resolution.hpp"
#include "resolution/resolution_internals.hpp"

#include "tests/tests.hpp"

#include "cli/cli.hpp"


namespace {

    constexpr struct {
        auto operator<<(auto const& arg) const -> std::ostream& {
            return std::cerr << bu::Color::red << "Error: " << bu::Color::white << arg;
        }
    } error_stream;


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
                    error_stream << exception.what() << '\n';
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

    [[maybe_unused]]
    auto const expression_resolution_repl = generic_repl([](bu::Source source) {
        auto tokenized_source = lexer::lex(std::move(source));
        parser::Parse_context parse_context { tokenized_source };
        auto result = parser::extract_expression(parse_context);

        resolution::Namespace space;
        resolution::Resolution_context resolution_context {
            .scope             = { .parent = nullptr },
            .current_namespace = &space,
            .global_namespace  = &space,
            .is_unevaluated    = false
        };
        auto expression = resolution::resolve_expression(result, resolution_context);

        bu::print("ir: {}\n", expression);
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
    cli::Options_description description;
    description.add_options()
        ("help"   ,                "Show this text"              )
        ("version",                "Show the interpreter version")
        ("repl"   , cli::string(), "Run the given repl"          )
        ("machine",                "Run the interpreter test"    )
        ("resolve",                "Run the resolution test"     )
        ("nocolor",                "Disable colored output"      );

    auto options = cli::parse_command_line(argc, argv, description);

    if (options.find("help")) {
        bu::print("Valid options:\n\n{}", description);
        return 0;
    }

    if (options.find("version")) {
        bu::print("Version {}, compiled on " __DATE__ ", " __TIME__ ".\n", vm::Virtual_machine::version);
    }

    if (options.find("nocolor")) {
        bu::disable_color_formatting();
    }

    tests::run_all_tests();

    if (options.find("resolve")) {
        auto pipeline = bu::compose(&resolution::resolve, &parser::parse, &lexer::lex);
        auto path     = std::filesystem::current_path() / "sample-project\\main.vmt";
        auto program  = pipeline(bu::Source { path.string() });

        std::ignore = program;
    }

    if (options.find("machine")) {
        bu::unimplemented(); // MSVC ICE

        /*vm::Virtual_machine machine { .stack = bu::Bytestack { 1000 } };

        bu::Timer timer;

        auto const string = machine.add_to_string_pool("Hello, world!\n");

        using enum vm::Opcode;
        machine.bytecode.write(
            ipush, 0_iz,
            iinc_top,
            idup,
            spush, string,
            sprint,
            local_jump_ineq_i, vm::Local_offset_type(-23), 500'000_iz,
            halt
        );

        return machine.run();*/
    }

    if (auto* const name = options.find_str("repl")) {
        if (*name == "lex") {
            lexer_repl();
        }
        else if (*name == "expr") {
            expression_parser_repl();
        }
        else if (*name == "prog") {
            program_parser_repl();
        }
        else if (*name == "res") {
            expression_resolution_repl();
        }
        else {
            bu::abort("Unrecognized repl name");
        }
    }
}

catch (cli::Unrecognized_option const& exception) {
    error_stream << exception.what() << "; use --help to see a list of valid options\n";
}

catch (std::exception const& exception) {
    error_stream << exception.what() << '\n';
}