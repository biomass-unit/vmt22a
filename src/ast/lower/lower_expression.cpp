#include "bu/utilities.hpp"
#include "lowering_internals.hpp"


namespace {

    struct Expression_lowering_visitor {
        Lowering_context     & context;
        ast::Expression const& this_expression;


        template <class T>
        auto operator()(ast::expression::Literal<T> const& literal) -> hir::Expression::Variant {
            return literal;
        }

        auto operator()(ast::expression::Array_literal const& literal) -> hir::Expression::Variant {
            return hir::expression::Array_literal {
                .elements = bu::map(context.lower())(literal.elements)
            };
        }

        auto operator()(ast::expression::Variable const& variable) -> hir::Expression::Variant {
            return hir::expression::Variable {
                .name = context.lower(variable.name)
            };
        }

        auto operator()(ast::expression::Tuple const& tuple) -> hir::Expression::Variant {
            return hir::expression::Tuple {
                .elements = bu::map(context.lower())(tuple.elements)
            };
        }

        auto operator()(ast::expression::Conditional const& conditional) -> hir::Expression::Variant {
            auto false_branch = conditional.false_branch
                              . transform(context.lower())
                              . value_or(context.unit_value(this_expression.source_view));

            if (auto* const let = std::get_if<ast::expression::Conditional_let>(&conditional.condition->value)) {
                /*
                    if let a = b { c } else { d }

                    is transformed into

                    match b {
                        a -> c
                        _ -> d
                    }
                */

                return hir::expression::Match {
                    .cases = bu::vector_from<hir::expression::Match::Case>({
                        {
                            .pattern = context.lower(*let->pattern),
                            .expression = context.lower(conditional.true_branch)
                        },
                        {
                            .pattern = hir::Pattern {
                                .value       = hir::pattern::Wildcard {},
                                .source_view = let->pattern->source_view
                            },
                            .expression = std::move(false_branch)
                        }
                    }),
                    .expression = context.lower(*let->initializer)
                };
            }
            else {
                /*
                    if a { b } else { c }

                    is transformed into

                    match a {
                        true -> b
                        false -> c
                    }
                */

                return hir::expression::Match {
                    .cases = bu::vector_from<hir::expression::Match::Case>({
                        {
                            .pattern = context.true_pattern(this_expression.source_view),
                            .expression = context.lower(conditional.true_branch)
                        },
                        {
                            .pattern = context.false_pattern(this_expression.source_view),
                            .expression = false_branch
                        }
                    }),
                    .expression = context.lower(conditional.condition)
                };
            }
        }

        auto operator()(ast::expression::Match const& match) -> hir::Expression::Variant {
            auto const lower_match_case = [this](ast::expression::Match::Case const& match_case)
                -> hir::expression::Match::Case
            {
                return {
                    .pattern    = context.lower(match_case.pattern),
                    .expression = context.lower(match_case.expression)
                };
            };

            return hir::expression::Match {
                .cases      = bu::map(lower_match_case)(match.cases),
                .expression = context.lower(match.expression)
            };
        }

        auto operator()(ast::expression::Block const& block) -> hir::Expression::Variant {
            return hir::expression::Block {
                .side_effects = bu::map(context.lower())(block.side_effects),
                .result       = block.result.transform(bu::compose(bu::wrap, context.lower()))
            };
        }

        auto operator()(ast::expression::While_loop const& loop) -> hir::Expression::Variant {
            if (auto* const let = std::get_if<ast::expression::Conditional_let>(&loop.condition->value)) {
                /*
                    while let a = b { c }

                    is transformed into

                    loop {
                        match b {
                            a -> c
                            _ -> break
                        }
                    }
                */

                return hir::expression::Loop {
                    .body = hir::Expression {
                        .value = hir::expression::Match {
                            .cases = bu::vector_from<hir::expression::Match::Case>({
                                {
                                    .pattern    = context.lower(*let->pattern),
                                    .expression = context.lower(loop.body)
                                },
                                {
                                    .pattern = context.wildcard_pattern(this_expression.source_view),
                                    .expression = hir::Expression {
                                        .value       = hir::expression::Break {},
                                        .source_view = this_expression.source_view
                                    }
                                }
                            }),
                            .expression = context.lower(*let->initializer)
                        },
                        .source_view = loop.body->source_view
                    }
                };
            }

            /*
                while a { b }

                is transformed into

                loop {
                    match a {
                        true -> b
                        false -> break
                    }
                }
            */

            return hir::expression::Loop {
                .body = hir::Expression {
                    .value = hir::expression::Match {
                        .cases = bu::vector_from<hir::expression::Match::Case>({
                            {
                                .pattern    = context.true_pattern(this_expression.source_view),
                                .expression = context.lower(loop.body)
                            },
                            {
                                .pattern = context.false_pattern(this_expression.source_view),
                                .expression = hir::Expression {
                                    .value       = hir::expression::Break {},
                                    .source_view = this_expression.source_view
                                }
                            }
                        }),
                        .expression = context.lower(loop.condition)
                    },
                    .source_view = loop.body->source_view
                }
            };
        }

        auto operator()(ast::expression::Infinite_loop const& loop) -> hir::Expression::Variant {
            return hir::expression::Loop { .body = context.lower(loop.body) };
        }

        auto operator()(ast::expression::Invocation const& invocation) -> hir::Expression::Variant {
            return hir::expression::Invocation {
                .arguments = bu::map(context.lower())(invocation.arguments),
                .invocable = context.lower(invocation.invocable)
            };
        }

        auto operator()(ast::expression::Struct_initializer const& initializer) -> hir::Expression::Variant {
            decltype(hir::expression::Struct_initializer::member_initializers) initializers;
            initializers.container().reserve(initializer.member_initializers.size());

            for (auto& [name, init] : initializer.member_initializers.span()) {
                initializers.add(bu::copy(name), context.lower(init));
            }

            return hir::expression::Struct_initializer {
                .member_initializers = std::move(initializers),
                .type                = context.lower(initializer.type)
            };
        }

        auto operator()(ast::expression::Binary_operator_invocation const& invocation) -> hir::Expression::Variant {
            return hir::expression::Binary_operator_invocation {
                .left  = context.lower(invocation.left),
                .right = context.lower(invocation.right),
                .op    = invocation.op
            };
        }

        auto operator()(ast::expression::Template_application const& application) -> hir::Expression::Variant {
            return hir::expression::Template_application {
                .template_arguments = bu::map(context.lower())(application.template_arguments),
                .name               = context.lower(application.name)
            };
        }

        auto operator()(ast::expression::Member_access_chain const& chain) -> hir::Expression::Variant {
            using AST_chain = ast::expression::Member_access_chain;
            using HIR_chain = hir::expression::Member_access_chain;

            auto const lower_accessor = [this](AST_chain::Accessor const& accessor) {
                return std::visit<HIR_chain::Accessor>(bu::Overload {
                    [](AST_chain::Tuple_field const& field) {
                        return HIR_chain::Tuple_field { .index = field.index };
                    },
                    [](AST_chain::Struct_field const& field) {
                        return HIR_chain::Struct_field { .name = field.name };
                    },
                    [this](AST_chain::Array_index const& index) {
                        return HIR_chain::Array_index { .expression = context.lower(index.expression) };
                    }
                }, accessor);
            };

            return hir::expression::Member_access_chain {
                .accessors  = bu::map(lower_accessor)(chain.accessors),
                .expression = context.lower(chain.expression)
            };
        }

        auto operator()(ast::expression::Member_function_invocation const& invocation) -> hir::Expression::Variant {
            return hir::expression::Member_function_invocation {
                .arguments   = bu::map(context.lower())(invocation.arguments),
                .expression  = context.lower(invocation.expression),
                .member_name = invocation.member_name
            };
        }

        auto operator()(ast::expression::Type_cast const& cast) -> hir::Expression::Variant {
            return hir::expression::Type_cast {
                .expression = context.lower(cast.expression),
                .target     = context.lower(cast.target),
                .kind       = cast.kind
            };
        }

        auto operator()(ast::expression::Let_binding const& let) -> hir::Expression::Variant {
            return hir::expression::Let_binding {
                .pattern     = context.lower(let.pattern),
                .initializer = context.lower(let.initializer),
                .type        = let.type.transform(bu::compose(bu::wrap, context.lower()))
            };
        }

        auto operator()(ast::expression::Local_type_alias const& alias) -> hir::Expression::Variant {
            return hir::expression::Local_type_alias {
                .name = alias.name,
                .type = context.lower(alias.type)
            };
        }

        auto operator()(ast::expression::Ret const& ret) -> hir::Expression::Variant {
            return hir::expression::Ret {
                .expression = ret.expression.transform(bu::compose(bu::wrap, context.lower()))
            };
        }

        auto operator()(ast::expression::Break const& break_) -> hir::Expression::Variant {
            return hir::expression::Break {
                .label      = break_.label,
                .expression = break_.expression.transform(bu::compose(bu::wrap, context.lower()))
            };
        }

        auto operator()(ast::expression::Continue const&) -> hir::Expression::Variant {
            return hir::expression::Continue {};
        }

        auto operator()(ast::expression::Size_of const& size_of) -> hir::Expression::Variant {
            return hir::expression::Size_of { .type = context.lower(size_of.type) };
        }

        auto operator()(ast::expression::Reference const& reference) -> hir::Expression::Variant {
            return hir::expression::Reference {
                .mutability = reference.mutability,
                .expression = context.lower(reference.expression)
            };
        }

        auto operator()(ast::expression::Dereference const& dereference) -> hir::Expression::Variant {
            return hir::expression::Dereference {
                .expression = context.lower(dereference.expression)
            };
        }

        auto operator()(ast::expression::Placement_init const& init) -> hir::Expression::Variant {
            return hir::expression::Placement_init {
                .lvalue      = context.lower(init.lvalue),
                .initializer = context.lower(init.initializer)
            };
        }

        auto operator()(ast::expression::Meta const& meta) -> hir::Expression::Variant {
            return hir::expression::Meta { .expression = context.lower(meta.expression) };
        }

        auto operator()(ast::expression::Hole const&) -> hir::Expression::Variant {
            return hir::expression::Hole {};
        }


        auto operator()(ast::expression::For_loop const&) -> hir::Expression::Variant {
            context.error(this_expression.source_view, { "For loops are not supported yet" });
        }

        auto operator()(ast::expression::Lambda const&) -> hir::Expression::Variant {
            context.error(this_expression.source_view, { "Lambda expressions are not supported yet" });
        }


        auto operator()(ast::expression::Conditional_let const&) -> hir::Expression::Variant {
            // Should be unreachable because a conditional let expression can
            // only occur as the condition of if-let or while-let expressions.
            bu::abort();
        }

    };

}


auto Lowering_context::lower(ast::Expression const& expression) -> hir::Expression {
    return {
        .value = std::visit(
            Expression_lowering_visitor {
                .context         = *this,
                .this_expression = expression
            },
            expression.value
        ),
        .source_view = expression.source_view
    };
}