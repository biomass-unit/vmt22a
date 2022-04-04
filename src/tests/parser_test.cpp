#include "bu/utilities.hpp"
#include "internals/test_internals.hpp"

#include "ast/ast_formatting.hpp"
#include "parser/internals/parser_internals.hpp"


namespace {

    auto assert_expr_eq(std::string_view text, auto&& expr, std::source_location caller = std::source_location::current()) -> void {
        tests::assert_eq(
            [&] {
                auto ts = lexer::lex(bu::Source { bu::Source::Mock_tag {}, std::string { text } });
                parser::Parse_context context { ts };
                return parser::extract_expression(context);
            }(),
            ast::Expression { std::forward<decltype(expr)>(expr) },
            caller
        );
    };

    auto unqualified(std::string_view name) {
        return ast::Qualified_name {
            .primary_qualifier {
                .name     = lexer::Identifier { name },
                .is_upper = std::isupper(name.front()) != 0
            }
        };
    };

}


auto run_parser_tests() -> void {
    using namespace lexer::literals;
    using namespace bu::literals;
    using namespace tests;


    "literal"_test = [] {
        assert_expr_eq(
            "4.2e3",
            ast::expression::Literal { 4.2e3 }
        );

        assert_expr_eq(
            "\"Hello, \"     \"world!\"",
            ast::expression::Literal { lexer::String { "Hello, world!"sv } }
        );
    };


    "conditional"_test = [] {
        assert_expr_eq(
            "if false { true; } else { 'a' }",
            ast::expression::Conditional {
                ast::expression::Literal { false },
                ast::expression::Compound {
                    {
                        ast::expression::Literal { true },
                        ast::unit_value
                    }
                },
                ast::expression::Compound {
                    {
                        ast::expression::Literal { static_cast<bu::Char>('a') }
                    }
                }
            }
        );
    };


    "conditional_elif"_test = [] {
        assert_expr_eq(
            "if true { 50 } elif false { 75 } else { 100 }",
            ast::expression::Conditional {
                ast::expression::Literal { true },
                ast::expression::Compound { { ast::expression::Literal { 50_iz } } },
                ast::expression::Conditional {
                    ast::expression::Literal { false },
                    ast::expression::Compound { { ast::expression::Literal { 75_iz } } },
                    ast::expression::Compound { { ast::expression::Literal { 100_iz } } },
                }
            }
        );
    };


    "for_loop"_test = [] {
        assert_expr_eq(
            "for x in \"hello\" {}",
            ast::expression::For_loop {
                ast::pattern::Name { "x"_id, { .type = ast::Mutability::Type::immut } },
                ast::expression::Literal { lexer::String { "hello"sv } },
                ast::expression::Compound {}
            }
        );
    };


    "operator_precedence"_test = [] {
        assert_expr_eq(
            "1 * 2 +$+ 3 + 4",
            ast::expression::Binary_operator_invocation {
                ast::Expression {
                    ast::expression::Binary_operator_invocation {
                        ast::expression::Literal<bu::Isize> { 1 },
                        ast::expression::Literal<bu::Isize> { 2 },
                        "*"_id
                    }
                },
                ast::Expression {
                    ast::expression::Binary_operator_invocation {
                        ast::expression::Literal<bu::Isize> { 3 },
                        ast::expression::Literal<bu::Isize> { 4 },
                        "+"_id
                    }
                },
                "+$+"_id
            }
        );
    };


    "type_cast"_test = [] {
        assert_expr_eq(
            "'x' as Int as Bool as Float + 3.14",
            ast::expression::Binary_operator_invocation {
                ast::expression::Type_cast {
                    ast::Expression {
                        ast::expression::Type_cast {
                            ast::Expression {
                                ast::expression::Type_cast {
                                    ast::expression::Literal<bu::Char> { 'x' },
                                    ast::type::integer
                                }
                            },
                            ast::type::boolean
                        }
                    },
                    ast::type::floating
                },
                ast::expression::Literal { 3.14 },
                "+"_id
            }
        );
    };


    "member_access"_test = [] {
        assert_expr_eq(
            "().1.2.x.50.y",
            ast::expression::Member_access_chain {
                .accessors { 1, 2, "x"_id, 50, "y"_id },
                .expression = ast::unit_value
            }
        );
    };


    "member_function"_test = [] {
        assert_expr_eq(
            "x.y.f()",
            ast::expression::Member_function_invocation {
                .arguments = {},
                .expression = ast::expression::Member_access_chain {
                    .accessors { "y"_id },
                    .expression = ast::expression::Variable { unqualified("x") }
                },
                .member_name = "f"_id
            }
        );
    };


    "let_binding"_test = [] {
        ast::Type type {
            ast::type::Typename {
                .identifier {
                    .middle_qualifiers {
                        ast::Qualifier {
                            .name = "std"_id,
                            .is_upper = false
                        },
                        ast::Qualifier {
                            .template_arguments = std::vector<ast::Template_argument> {
                                {
                                    ast::Type {
                                        ast::type::Typename {
                                            unqualified("Long")
                                        }
                                    }
                                }
                            },
                            .name = "Vector"_id,
                            .is_upper = true
                        }
                    },
                    .primary_qualifier { .name = "Element"_id, .is_upper = true }
                }
            }
        };

        assert_expr_eq(
            "let _: std::Vector[Long]::Element = 5",
            ast::expression::Let_binding{
                .pattern     = ast::pattern::Wildcard {},
                .initializer = ast::expression::Literal { 5_iz },
                .type        = std::move(type), // Writing the type inline caused a MSVC ICE
            }
        );
    };

}