#include "bu/utilities.hpp"
#include "bu/color.hpp"
#include "bu/timer.hpp"

#include "lexer/lexer.hpp"

#include "parser/parser.hpp"
#include "parser/parser_internals.hpp"

#include "ast/ast.hpp"
#include "ast/lower/lower.hpp"

#include "resolution/resolution.hpp"

#include "vm/bytecode.hpp"
#include "vm/virtual_machine.hpp"
#include "vm/vm_formatting.hpp"

#include "tests/tests.hpp"

#include "cli/cli.hpp"

#include "language/configuration.hpp"


namespace {

    auto error_stream() -> std::ostream& {
        return std::cerr << bu::Color::red << "Error: " << bu::Color::white;
    }


    consteval auto generic_repl(void (*f)(bu::Source)) {
        return [=] {
            for (;;) {
                std::string string;

                bu::print(" >>> ");
                std::getline(std::cin, string);

                if (string.empty()) {
                    continue;
                }
                else if (string == "q") {
                    break;
                }

                try {
                    bu::Source source { bu::Source::Mock_tag { "repl" }, std::move(string) };
                    f(std::move(source));
                }
                catch (bu::diagnostics::Error const& error) {
                    std::cerr << error.what() << "\n\n";
                }
                catch (std::exception const& exception) {
                    error_stream() << exception.what() << "\n\n";
                }
            }
        };
    }

    [[maybe_unused]]
    constexpr auto lexer_repl = generic_repl([](bu::Source source) {
        bu::print("Tokens: {}\n", lexer::lex(std::move(source)).tokens);
    });

    [[maybe_unused]]
    constexpr auto expression_parser_repl = generic_repl([](bu::Source source) {
        auto tokenized_source = lexer::lex(std::move(source));
        parser::Parse_context context { tokenized_source };

        if (auto result = parser::parse_expression(context)) {
            bu::print("Result: {}\n", result);

            if (!context.pointer->source_view.string.empty()) {
                bu::print("Remaining input: '{}'\n", context.pointer->source_view.string.data());
            }
        }
        else {
            bu::print("No parse\n");
        }
    });

    [[maybe_unused]]
    constexpr auto program_parser_repl = generic_repl([](bu::Source source) {
        bu::print("{}\n", parser::parse(lexer::lex(std::move(source))));
    });

    [[maybe_unused]]
    constexpr auto lowering_repl = generic_repl([](bu::Source source) {
        auto module = ast::lower(parser::parse(lexer::lex(std::move(source))));
        bu::print("{}\n\n", bu::fmt::delimited_range(module.definitions, "\n\n"));
    });


    auto initialize_project(std::string_view const project_name) -> void {
        auto parent_path  = std::filesystem::current_path();
        auto project_path = parent_path / project_name;
        auto source_dir   = project_path / "src";

        if (project_path.has_extension()) {
            throw bu::exception("A directory name can not have a file extension");
        }

        if (is_directory(project_path)) {
            throw bu::exception(
                "A directory with the path '{}' already exists. Please use a new name",
                project_path.string()
            );
        }

        if (!create_directory(project_path)) {
            throw bu::exception(
                "Could not create a directory with the path '{}'",
                project_path.string()
            );
        }

        {
            std::ofstream configuration_file { project_path / "vmt22a_config" };
            if (!configuration_file) {
                throw bu::exception("Could not create the configuration file");
            }
            configuration_file << language::default_configuration().string();
        }

        if (!create_directory(source_dir)) {
            throw bu::exception("Could not create the source directory");
        }

        {
            std::ofstream main_file { source_dir / "main.vmt" };
            if (!main_file) {
                throw bu::exception("Could not create the main file");
            }
            main_file << "import std\n\nfn main() {\n    print(\"Hello, world!\\n\")\n}";
        }

        bu::print("Successfully created a new project at '{}'\n", project_path.string());
    }

}


using namespace bu    :: literals;
using namespace lexer :: literals;


auto main(int argc, char const** argv) -> int try {
    bu::do_empty_wrapper_arena_debug_messages = false;

    cli::Options_description description;
    description.add_options()
        ("help"   ,                      "Show this text"          )
        ("version",                      "Show vmt version"        )
        ("new"    , cli::string("name"), "Create a new vmt project")
        ("repl"   , cli::string("name"), "Run the given repl"      )
        ("machine"                                                 )
        ("resolve"                                                 )
        ("nocolor",                      "Disable colored output"  )
        ("time"   ,                      "Print the execution time")
        ("test"   ,                      "Run all tests"           );

    cli::Options options = bu::expect(cli::parse_command_line(argc, argv, description));


    auto configuration = language::read_configuration();
    (void)configuration;

    bu::Logging_timer execution_timer { [&options](bu::Logging_timer::Duration const elapsed) {
            if (options["time"]) {
                bu::print("Total execution time: {}\n", elapsed);
            }
        }
    };

    if (options["nocolor"]) {
        bu::disable_color_formatting();
    }

    if (options["help"]) {
        bu::print("Valid options:\n\n{}", description);
        return 0;
    }

    if (options["version"]) {
        bu::print("vmt22a version {}, compiled on " __DATE__ ", " __TIME__ ".\n", language::version);
    }

    if (std::string_view const* const name = options["new"]) {
        initialize_project(*name);
    }

    if (options["test"]) {
        tests::run_all_tests();
    }

    if (options["machine"]) {
        vm::Virtual_machine machine { .stack = bu::Bytestack { 32 } };

        auto const string = machine.program.constants.add_to_string_pool("Hello, world!\n");

        using enum vm::Opcode;
        machine.program.bytecode.write(
            ipush, 0_iz,
            iinc_top,
            idup,
            spush, string,
            sprint,
            local_jump_ineq_i, vm::Local_offset_type(-23), 10_iz,
            halt
        );

        return machine.run();
    }

    if (options["resolve"]) {
        bu::Source source { (std::filesystem::current_path() / "sample-project" / "src" / "main.vmt").string() };
        [[maybe_unused]] auto module = resolution::resolve(ast::lower(parser::parse(lexer::lex(std::move(source)))));
    }

    if (cli::types::Str const* const name = options["repl"]) {
        if (*name == "lex") {
            lexer_repl();
        }
        else if (*name == "expr") {
            ast::Node_context repl_context;
            expression_parser_repl();
        }
        else if (*name == "prog") {
            program_parser_repl();
        }
        else if (*name == "lower") {
            lowering_repl();
        }
        else {
            bu::abort("Unrecognized repl name");
        }
    }
}

catch (cli::Unrecognized_option const& exception) {
    std::cerr << exception.what() << "; use --help to see a list of valid options\n";
}

catch (bu::diagnostics::Error const& error) {
    std::cerr << error.what() << '\n';
}

catch (std::exception const& exception) {
    error_stream() << exception.what() << '\n';
}