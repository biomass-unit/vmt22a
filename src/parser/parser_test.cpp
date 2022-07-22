#include "bu/utilities.hpp"
#include "tests/tests.hpp"

#include "parser/parser_internals.hpp"


namespace {

    constexpr auto empty_view() {
        return bu::Source_view { {}, {}, {} };
    }

    auto mk_expr(ast::Expression::Variant&& expr) -> ast::Expression {
        return { .value = std::move(expr), .source_view = empty_view() };
    }

    auto mk_patt(ast::Pattern::Variant&& patt) -> ast::Pattern {
        return { .value = std::move(patt), .source_view = empty_view() };
    }

    auto mk_type(ast::Type::Variant&& type) -> ast::Type {
        return { .value = std::move(type), .source_view = empty_view() };
    }


    template <auto extract>
    auto extract_node(std::string_view const text) {
        auto ts = lexer::lex(
            bu::Source {
                bu::Source::Mock_tag { .filename = "test" },
                std::string(text)
            },
            bu::diagnostics::Builder { {
                bu::diagnostics::Level::suppress,
                bu::diagnostics::Level::suppress,
            } }
        );

        parser::Parse_context context { ts };
        auto value = extract(context);

        if (!context.is_finished()) {
            throw tests::Failure {
                std::format(
                    "Remaining input: '{}'",
                    context.pointer->source_view.string.data()
                )
            };
        }

        return value;
    }

    constexpr auto extract_expression = extract_node<parser::parse_expression>;
    constexpr auto extract_pattern = extract_node<parser::parse_pattern>;
    constexpr auto extract_type = extract_node<parser::parse_type>;



    template <auto extract, class Node = std::invoke_result_t<decltype(extract), parser::Parse_context&>>
    auto assert_node_eq(std::string_view const text, typename Node::Variant&& value, std::source_location const caller = std::source_location::current()) -> void {
        tests::assert_eq(
            extract_node<extract>(text),
            Node { .value = std::move(value), .source_view = empty_view() },
            caller
        );
    };

    auto assert_expr_eq(std::string_view const text, ast::Expression::Variant&& value, std::source_location const caller = std::source_location::current()) -> void {
        assert_node_eq<parser::extract_expression>(text, std::move(value), caller);
    }
    auto assert_patt_eq(std::string_view const text, ast::Pattern::Variant&& value, std::source_location const caller = std::source_location::current()) -> void {
        assert_node_eq<parser::extract_pattern>(text, std::move(value), caller);
    }
    auto assert_type_eq(std::string_view const text, ast::Type::Variant&& value, std::source_location const caller = std::source_location::current()) -> void {
        assert_node_eq<parser::extract_type>(text, std::move(value), caller);
    }


    auto operator"" _unqualified(char const* const s, bu::Usize const n)
        -> ast::Qualified_name
    {
        return ast::Qualified_name {
            .primary_name {
                .identifier  = lexer::Identifier { std::string_view { s, n } },
                .is_upper    = std::isupper(*s) != 0,
                .source_view = empty_view()
            }
        };
    }


    auto run_parser_tests() -> void {
        using namespace lexer::literals;
        using namespace bu::literals;
        using namespace tests;

        ast::Node_context test_context;


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

        "block"_test = [] {
            assert_expr_eq(
                "{}",
                ast::expression::Block {}
            );

            assert_expr_eq(
                "{ {} }",
                ast::expression::Block {
                    .result = mk_expr(ast::expression::Block {})
                }
            );

            assert_expr_eq(
                "{ {}; }",
                ast::expression::Block {
                    .side_effects { mk_expr(ast::expression::Block {}) }
                }
            );

            assert_expr_eq(
                "{ {}; {} }",
                ast::expression::Block {
                    .side_effects { mk_expr(ast::expression::Block {}) },
                    .result = mk_expr(ast::expression::Block {})
                }
            );
        };

        "conditional"_test = [] {
            assert_expr_eq(
                "if false { true; } else { 'a' }",
                ast::expression::Conditional {
                    mk_expr(ast::expression::Literal { false }),
                    mk_expr(ast::expression::Block {
                        .side_effects { mk_expr(ast::expression::Literal { true }) }
                    }),
                mk_expr(ast::expression::Block {
                    .result = mk_expr(ast::expression::Literal { static_cast<bu::Char>('a') })
                })
            });
        };

        "conditional_elif"_test = [] {
            assert_expr_eq(
                "if true { 50 } elif false { 75 } else { 100 }",
                ast::expression::Conditional {
                    mk_expr(ast::expression::Literal { true }),
                    mk_expr(ast::expression::Block {
                        .result = mk_expr(ast::expression::Literal { 50_iz })
                    }),
                    mk_expr(ast::expression::Conditional {
                        mk_expr(ast::expression::Literal { false }),
                        mk_expr(ast::expression::Block {
                            .result = mk_expr(ast::expression::Literal { 75_iz })
                        }),
                        mk_expr(ast::expression::Block {
                            .result = mk_expr(ast::expression::Literal { 100_iz })
                        }),
                    })
                }
            );
        };

        "for_loop"_test = [] {
            assert_expr_eq(
                "outer for x in \"hello\" {}",
                ast::expression::For_loop {
                    .label = ast::Name {
                        .identifier = "outer"_id,
                        .is_upper = false,
                        .source_view = empty_view()
                    },
                    .iterator = mk_patt(ast::pattern::Name {
                        "x"_id,
                        {
                            .type = ast::Mutability::Type::immut,
                            .source_view = empty_view()
                        }
                    }),
                    .iterable = mk_expr(ast::expression::Literal { lexer::String { "hello"sv } }),
                    .body = mk_expr(ast::expression::Block {})
                }
            );
        };

        "operator_precedence"_test = [] {
            assert_expr_eq(
                "1 * 2 +$+ 3 + 4",
                ast::expression::Binary_operator_invocation {
                    mk_expr(ast::expression::Binary_operator_invocation {
                        mk_expr(ast::expression::Literal<bu::Isize> { 1 }),
                        mk_expr(ast::expression::Literal<bu::Isize> { 2 }),
                        "*"_id
                    }),
                    mk_expr(ast::expression::Binary_operator_invocation {
                        mk_expr(ast::expression::Literal<bu::Isize> { 3 }),
                        mk_expr(ast::expression::Literal<bu::Isize> { 4 }),
                        "+"_id
                    }),
                    "+$+"_id
                }
            );
        };

        "type_cast"_test = [] {
            assert_expr_eq(
                "'x' as Int as Bool as Float + 3.14",
                ast::expression::Binary_operator_invocation {
                    mk_expr(ast::expression::Type_cast {
                        mk_expr(ast::expression::Type_cast {
                            mk_expr(ast::expression::Type_cast {
                                mk_expr(ast::expression::Literal<bu::Char> { 'x' }),
                                mk_type(ast::type::Integer {})
                            }),
                            mk_type(ast::type::Boolean {})
                        }),
                        mk_type(ast::type::Floating {})
                    }),
                    mk_expr(ast::expression::Literal { 3.14 }),
                    "+"_id
                }
            );
        };

        "member_access"_test = [] {
            assert_expr_eq(
                "().1.2.x.50.y",
                ast::expression::Member_access_chain {
                    .accessors { 1, 2, "x"_id, 50, "y"_id },
                    .expression = mk_expr(ast::expression::Tuple {})
                }
            );
        };

        "member_function"_test = [] {
            assert_expr_eq(
                "x.y.f()",
                ast::expression::Member_function_invocation {
                    .arguments = {},
                    .expression = mk_expr(ast::expression::Member_access_chain {
                        .accessors { "y"_id },
                        .expression = mk_expr(ast::expression::Variable { "x"_unqualified })
                    }),
                    .member_name = "f"_id
                }
            );
        };

        "let_binding"_test = [] {
            auto type = mk_type(
                ast::type::Typename {
                    .identifier {
                        .middle_qualifiers {
                            ast::Qualifier {
                                .name {
                                    .identifier  = "std"_id,
                                    .is_upper    = false,
                                    .source_view = empty_view()
                                },
                                .source_view = empty_view()
                            },
                            ast::Qualifier {
                                .template_arguments = std::vector<ast::Template_argument> {
                                    {
                                        bu::wrap(mk_type(ast::type::Typename { "Long"_unqualified }))
                                    }
                                },
                                .name {
                                    .identifier  = "Vector"_id,
                                    .is_upper    = true,
                                    .source_view = empty_view()
                                },
                                .source_view = empty_view()
                            }
                        },
                        .primary_name {
                            .identifier  = "Element"_id,
                            .is_upper    = true,
                            .source_view = empty_view()
                        }
                    }
                }
            );

            assert_expr_eq(
                "let _: std::Vector[Long]::Element = 5",
                ast::expression::Let_binding {
                    .pattern     = mk_patt(ast::pattern::Wildcard {}),
                    .initializer = mk_expr(ast::expression::Literal { 5_iz }),
                    .type        = std::move(type), // Writing the type inline caused a MSVC ICE
                }
            );
        };

        "implicit_tuple_let_binding"_test = [] {
            assert_expr_eq(
                "let a, mut b: (Int, Float) = (10, 20.5)",
                ast::expression::Let_binding {
                    .pattern = mk_patt(ast::pattern::Tuple {
                        .patterns = bu::vector_from({
                            mk_patt(ast::pattern::Name {
                                .identifier = "a"_id,
                                .mutability { .type = ast::Mutability::Type::immut, .source_view = empty_view() }
                            }),
                            mk_patt(ast::pattern::Name {
                                .identifier = "b"_id,
                                .mutability { .type = ast::Mutability::Type::mut, .source_view = empty_view() }
                            })
                        })
                    }),
                    .initializer = mk_expr(ast::expression::Tuple {
                        .elements = bu::vector_from({
                            mk_expr(ast::expression::Literal<bu::Isize> { 10 }),
                            mk_expr(ast::expression::Literal<bu::Float> { 20.5 })
                        })
                    }),
                    .type = mk_type(ast::type::Tuple {
                        .types = bu::vector_from({
                            mk_type(ast::type::Integer {}),
                            mk_type(ast::type::Floating {})
                        })
                    })
                }
            );
        };

        "caseless_match"_throwing_test = [] {
            assert_expr_eq(
                "match 0 {}",
                ast::expression::Tuple {} // Doesn't matter
            );
        };

        "match"_test = [] {
            assert_expr_eq(
                "match x { 0 -> \"zero\" _ -> \"other\" }",
                ast::expression::Match {
                    .cases = bu::vector_from<ast::expression::Match::Case>({
                        {
                            .pattern = mk_patt(ast::pattern::Literal<bu::Isize> { 0 }),
                            .expression = mk_expr(ast::expression::Literal { lexer::String { "zero"sv } })
                        },
                        {
                            .pattern = mk_patt(ast::pattern::Wildcard {}),
                            .expression = mk_expr(ast::expression::Literal { lexer::String { "other"sv } })
                        },
                    }),
                    .expression = mk_expr(ast::expression::Variable { .name = "x"_unqualified }),
                }
            );
        };

        "implicit_tuple_case_match"_test = [] {
            assert_expr_eq(
                "match 0 { _, mut b, (c, _), [_] -> 1 }",
                ast::expression::Match {
                    .cases = bu::vector_from<ast::expression::Match::Case>({
                        {
                            .pattern = mk_patt(ast::pattern::Tuple {
                                .patterns = bu::vector_from({
                                    mk_patt(ast::pattern::Wildcard {}),
                                    mk_patt(ast::pattern::Name {
                                        .identifier = "b"_id,
                                        .mutability { .type = ast::Mutability::Type::mut, .source_view = empty_view() }
                                    }),
                                    mk_patt(ast::pattern::Tuple {
                                        .patterns = bu::vector_from({
                                            mk_patt(ast::pattern::Name {
                                                .identifier = "c"_id,
                                                .mutability { .type = ast::Mutability::Type::immut, .source_view = empty_view() }
                                            }),
                                            mk_patt(ast::pattern::Wildcard {})
                                        })
                                    }),
                                    mk_patt(ast::pattern::Slice {
                                        .patterns = bu::vector_from({
                                            mk_patt(ast::pattern::Wildcard {})
                                        })
                                    })
                                })
                            }),
                            .expression = mk_expr(ast::expression::Literal<bu::Isize> { 1 })
                        }
                    }),
                    .expression = mk_expr(ast::expression::Literal<bu::Isize> { 0 })
                }
            );
        };


        "tuple_type"_test = [] {
            assert_type_eq("()", ast::type::Tuple {});
            assert_type_eq("(())", ast::type::Tuple {});
            assert_type_eq(
                "(type_of(5), T)",
                ast::type::Tuple {
                    .types {
                        mk_type(ast::type::Type_of {
                            mk_expr(ast::expression::Literal<bu::Isize> { 5 })
                        }),
                        mk_type(ast::type::Typename { "T"_unqualified })
                    }
                }
            );
        };


        "tuple_pattern"_test = [] {
            assert_patt_eq("()", ast::pattern::Tuple {});
            assert_patt_eq("(())", ast::pattern::Tuple {});
            assert_patt_eq(
                "(x, _)",
                ast::pattern::Tuple {
                    .patterns {
                        mk_patt(ast::pattern::Name {
                            .identifier = "x"_id,
                            .mutability {
                                .type        = ast::Mutability::Type::immut,
                                .source_view = empty_view()
                            }
                        }),
                        mk_patt(ast::pattern::Wildcard {})
                    }
                }
            );
        };

        "enum_constructor_pattern"_test = [] {
            std::vector<ast::Qualifier> qualifiers {
                ast::Qualifier {
                    .name {
                        .identifier  = "Maybe"_id,
                        .is_upper    = true,
                        .source_view = empty_view()
                    },
                    .source_view = empty_view()
                }
            };

            assert_patt_eq(
                "Maybe::just(x)",
                ast::pattern::Constructor {
                    .name = ast::Qualified_name {
                        .middle_qualifiers = std::move(qualifiers),
                        .primary_name {
                            .identifier  = "just"_id,
                            .is_upper    = false,
                            .source_view = empty_view()
                        }
                    },
                    .pattern = mk_patt(ast::pattern::Name {
                        .identifier = "x"_id,
                        .mutability {
                            .type        = ast::Mutability::Type::immut,
                            .source_view = empty_view()
                        }
                    })
                }
            );
        };

        "enum_constructor_pattern"_throwing_test = [] {
            assert_patt_eq("Maybe::Just", ast::pattern::Tuple { /* doesn't matter */ });
        };

        "as_pattern"_test = [] {
            assert_patt_eq(
                "(_, _) as mut x",
                ast::pattern::As {
                    .name = ast::pattern::Name {
                        .identifier = "x"_id,
                        .mutability {
                            .type        = ast::Mutability::Type::mut,
                            .source_view = empty_view()
                        }
                    },
                    .pattern = mk_patt(ast::pattern::Tuple {
                        .patterns {
                            mk_patt(ast::pattern::Wildcard {}),
                            mk_patt(ast::pattern::Wildcard {})
                        }
                    })
                }
            );
        };


        "scope_access_1"_throwing_test = [] { std::ignore = extract_expression("::"); };

        "scope_access_2"_throwing_test = [] { std::ignore = extract_expression("test::"); };

        "scope_access_3"_test = [] { std::ignore = extract_expression("::test"); };

    }

}


REGISTER_TEST(run_parser_tests);