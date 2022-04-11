#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Expression_resolution_visitor {
        resolution::Resolution_context& context;
        ast::Expression&                this_expression;


        [[nodiscard]]
        auto error(std::string_view const message, std::string_view const help, ast::Expression& expression)
            -> std::runtime_error
        {
            return context.error(expression.source_view, message, help);
        }

        [[nodiscard]]
        auto error(std::string_view const message, std::optional<std::string_view> const help = std::nullopt)
            -> std::runtime_error
        {
            return context.error(this_expression.source_view, message, help);
        }

        [[nodiscard]]
        auto error(std::string_view const message, ast::Expression& expression)
            -> std::runtime_error
        {
            return context.error(expression.source_view, message, std::nullopt);
        }



        template <class T>
        auto operator()(ast::expression::Literal<T>& literal) -> ir::Expression {
            return {
                .value = literal,
                .type = ir::type::dtl::make_primitive<T>
            };
        }

        auto operator()(ast::expression::Array_literal& array) -> ir::Expression {
            std::vector<ir::Expression> elements;
            elements.reserve(array.elements.size());

            if (array.elements.empty()) {
                bu::unimplemented();
            }

            elements.push_back(context.resolve_expression(array.elements.front()));
            auto& head = elements.front();

            for (auto& element : array.elements | std::views::drop(1)) {
                auto elem = context.resolve_expression(element);

                if (head.type != elem.type) {
                    throw error(
                        std::format(
                            "The earlier elements in the array were of type {}, not {}",
                            head.type,
                            elem.type
                        ),
                        element
                    );
                }

                elements.push_back(std::move(elem));
            }

            auto const length = elements.size();
            auto const size   = head.type->size.copy().safe_mul(length);

            return {
                .value = ir::expression::Array_literal {
                    std::move(elements)
                },
                .type = ir::Type {
                    .value = ir::type::Array {
                        .element_type = head.type,
                        .length       = length
                    },
                    .size = size
                }
            };
        }

        auto operator()(ast::expression::Tuple& tuple) -> ir::Expression {
            std::vector<ir::Expression> expressions;
            std::vector<bu::Wrapper<ir::Type>> types;
            ir::Size_type size;
            bool          is_trivial = true;

            expressions.reserve(tuple.expressions.size());
            types      .reserve(tuple.expressions.size());

            for (auto& expression : tuple.expressions) {
                auto ir_expr = context.resolve_expression(expression);

                if (!ir_expr.type->is_trivial) {
                    is_trivial = false;
                }
                size.safe_add(ir_expr.type->size.get());

                types.push_back(ir_expr.type);
                expressions.push_back(std::move(ir_expr));
            }

            return {
                .value = ir::expression::Tuple { std::move(expressions) },
                .type = ir::Type {
                    .value      = ir::type::Tuple { std::move(types) },
                    .size       = size,
                    .is_trivial = is_trivial
                }
            };
        }

        auto operator()(ast::expression::Invocation& invocation) -> ir::Expression {
            auto invocable = context.resolve_expression(invocation.invocable);

            if (auto* const function = std::get_if<ir::type::Function>(&invocable.type->value)) {
                if (function->parameter_types.size() == invocation.arguments.size()) {
                    std::vector<ir::Expression> arguments;
                    arguments.reserve(invocation.arguments.size());

                    for (bu::Usize i = 0; i != invocation.arguments.size(); ++i) {
                        auto argument = context.resolve_expression(invocation.arguments[i].expression);
                        assert(!invocation.arguments[i].name);

                        if (argument.type == function->parameter_types[i]) {
                            arguments.push_back(std::move(argument));
                        }
                        else {
                            throw error(
                                std::format(
                                    "The {} parameter is of type {}, but the given argument is of type {}",
                                    bu::fmt::integer_with_ordinal_indicator(i + 1),
                                    function->parameter_types[i],
                                    argument.type
                                ),
                                invocation.arguments[i].expression
                            );
                        }
                    }

                    return {
                        .value = ir::expression::Invocation {
                            .arguments = std::move(arguments),
                            .invocable = std::move(invocable)
                        },
                        .type = function->return_type
                    };
                }
                else {
                    throw error(
                        std::format(
                            "The function (of type {}) takes {} "
                            "parameters, but {} arguments were supplied",
                            invocable.type,
                            function->parameter_types.size(),
                            invocation.arguments.size()
                        )
                    );
                }
            }
            else {
                throw error(std::format("{} is not invocable", invocable.type));
            }
        }

        auto operator()(ast::expression::Struct_initializer& struct_initializer) -> ir::Expression {
            bu::wrapper auto type = context.resolve_type(struct_initializer.type);

            if (auto* const uds = std::get_if<ir::type::User_defined_struct>(&type->value)) {
                auto& structure = *uds->structure;

                std::vector<ir::Expression> member_initializers;
                member_initializers.reserve(structure.members.size());

                for (auto& [name, member] : structure.members.container()) {
                    auto const count = std::ranges::count(
                        struct_initializer.member_initializers.container(),
                        name,
                        bu::first
                    );

                    if (count == 1) {
                        bu::wrapper auto* const initializer = struct_initializer.member_initializers.find(name);
                        assert(initializer); // Since count == 1, the initializer should be found

                        auto expression = context.resolve_expression(*initializer);

                        if (member.type == expression.type) {
                            member_initializers.push_back(std::move(expression));
                        }
                        else {
                            throw error(
                                std::format(
                                    "{} is of type {}, but the initializer is of type {}",
                                    name,
                                    member.type,
                                    expression.type
                                ),
                                *initializer
                            );
                        }
                    }
                    else {
                        throw error(
                            std::format(
                                "The struct initializer contains {} initializers "
                                "for {}, but exactly one is required",
                                count,
                                name
                            )
                        );
                    }
                }

                for (auto& [name, initializer] : struct_initializer.member_initializers.container()) {
                    if (!structure.members.find(name)) {
                        throw context.error(
                            initializer->source_view,
                            std::format("{} contains no member {}", structure.name, name)
                        );
                    }
                }

                return {
                    .value = ir::expression::Struct_initializer {
                        .member_initializers = std::move(member_initializers),
                        .type                = type
                    },
                    .type = type
                };
            }
            else {
                throw context.error(
                    struct_initializer.type->source_view,
                    std::format("{} is not a struct type", type)
                );
            }
        }

        auto operator()(ast::expression::Let_binding& let_binding) -> ir::Expression {
            auto initializer = context.resolve_expression(let_binding.initializer);

            if (let_binding.type) {
                bu::wrapper auto explicit_type =
                    context.resolve_type(*let_binding.type);

                if (explicit_type != initializer.type) {
                    throw error(
                        std::format(
                            "The binding is explicitly specified to be of "
                            "type {}, but the initializer is of type {}",
                            explicit_type,
                            initializer.type
                        ),
                        let_binding.initializer
                    );
                }
            }

            context.bind(let_binding.pattern, initializer.type);

            return {
                .value = ir::expression::Let_binding { std::move(initializer) },
                .type = ir::type::unit
            };
        }

        auto operator()(ast::expression::Local_type_alias& alias) -> ir::Expression {
            auto& bindings = context.scope.local_type_aliases.container();
            auto  it       = std::ranges::find(bindings, alias.name, bu::first);

            bu::wrapper auto type = context.resolve_type(alias.type);

            if (it != bindings.end() && !it->second.has_been_mentioned) {
                throw error(std::format("{} would shadow an unused local type alias", alias.name));
            }

            bindings.emplace(
                it,
                alias.name,
                resolution::Local_type_alias {
                    .type               = type,
                    .has_been_mentioned = false
                }
            );

            return {
                .value = ir::expression::Tuple {},
                .type  = ir::type::unit
            };
        }

        auto operator()(ast::expression::Variable& variable) -> ir::Expression {
            return context.find_variable_or_function(variable.name, this_expression, std::nullopt);
        }

        auto operator()(ast::expression::Template_instantiation& instantiation) -> ir::Expression {
            return context.find_variable_or_function(instantiation.name, this_expression, instantiation.template_arguments);
        }

        auto operator()(ast::expression::Take_reference& take_reference) -> ir::Expression {
            bool const take_mutable_ref =
                context.resolve_mutability(take_reference.mutability);

            if (auto* binding = context.scope.find(take_reference.name)) {
                if (take_mutable_ref && !binding->is_mutable) {
                    throw error("Can not acquire a mutable reference to an immutable binding");
                }

                binding->has_been_mentioned = true;

                return {
                    .value = ir::expression::Reference {
                        .frame_offset = binding->frame_offset
                    },
                    .type = ir::Type {
                        .value = ir::type::Reference {
                            ir::type::Pointer {
                                .type = binding->type,
                                .mut  = take_mutable_ref
                            }
                        },
                        .size       = ir::Size_type { bu::unchecked_tag, sizeof(std::byte*) },
                        .is_trivial = true
                    }
                };
            }
            else {
                throw error(std::format("{} is not a local variable", take_reference.name));
            }
        }

        auto operator()(ast::expression::Size_of& size_of) -> ir::Expression {
            bool const is_unevaluated = std::exchange(context.is_unevaluated, true);
            bu::wrapper auto type = context.resolve_type(size_of.type);
            context.is_unevaluated = is_unevaluated;

            return {
                .value = ir::expression::Literal<bu::Isize> {
                    type->size.safe_cast<bu::Isize>()
                },
                .type = ir::type::integer
            };
        }

        auto operator()(ast::expression::Member_access_chain& chain) -> ir::Expression {
            auto             expression       = context.resolve_expression(chain.expression);
            bu::wrapper auto most_recent_type = expression.type;

            bu::Bounded_integer<bu::U16> offset;

            for (auto& accessor : chain.accessors) {
                std::visit(bu::Overload {
                    [&](bu::Isize const member_index) {
                        if (member_index < 0) {
                            throw error("Negative tuple member indices are not allowed");
                        }

                        bu::Usize const index = static_cast<bu::Usize>(member_index);

                        if (auto* const tuple = std::get_if<ir::type::Tuple>(&most_recent_type->value)) {
                            if (index < tuple->types.size()) {
                                for (auto& type : tuple->types | std::views::take(member_index)) {
                                    offset.safe_add(type->size);
                                }
                                most_recent_type = tuple->types.at(index);
                            }
                            else {
                                throw error(
                                    std::format(
                                        "Tuple member index out of range; the tuple only contains {} elements",
                                        tuple->types.size()
                                    )
                                );
                            }
                        }
                        else {
                            throw error(std::format("{} is not a tuple type", most_recent_type)); // Improve
                        }
                    },
                    [&](lexer::Identifier const member_name) {
                        if (auto* const uds = std::get_if<ir::type::User_defined_struct>(&most_recent_type->value)) {
                            if (auto* const member = uds->structure->members.find(member_name)) {
                                most_recent_type = member->type;
                                offset.safe_add(member->offset);
                            }
                            else {
                                throw error(
                                    std::format(
                                        "{} does not contain a member {}",
                                        uds->structure->name,
                                        member_name
                                    )
                                );
                            }
                        }
                        else {
                            throw error(
                                std::format(
                                    "{} is not a struct type",
                                    most_recent_type
                                )
                            );
                        }
                    }
                }, accessor);
            }

            return {
                .value = ir::expression::Member_access {
                    std::move(expression),
                    offset
                },
                .type = most_recent_type
            };
        }

        auto operator()(ast::expression::Conditional& conditional) -> ir::Expression {
            auto condition = context.resolve_expression(conditional.condition);

            if (!std::holds_alternative<ir::type::Boolean>(condition.type->value)) {
                throw error(
                    std::format(
                        "The condition must be of type Bool, not {}",
                        condition.type
                    ),
                    conditional.condition
                );
            }

            auto true_branch = context.resolve_expression(conditional.true_branch);

            auto false_branch = conditional.false_branch
                              . transform(bu::compose(bu::wrap, resolution::resolve_expression_with(context)))
                              . value_or(ir::unit_value);

            if (true_branch.type != false_branch->type) {
                throw error(
                    std::format(
                        "The true-branch is of type {}, but the false branch is of type {}",
                        true_branch.type,
                        false_branch->type
                    ),
                    "Both of the branches must be of the same type"
                );
            }

            return {
                .value = ir::expression::Conditional {
                    std::move(condition),
                    std::move(true_branch),
                    std::move(false_branch)
                },
                .type = true_branch.type
            };
        }

        auto operator()(ast::expression::Type_cast& cast) -> ir::Expression {
            ir::expression::Type_cast ir_cast {
                .expression = context.resolve_expression(cast.expression),
                .type       = context.resolve_type(cast.target)
            };

            bu::wrapper auto type = ir_cast.type;

            return {
                .value = std::move(ir_cast),
                .type  = type
            };
        }

        auto ensure_loop_body_is_of_unit_type(ast::Expression& body, ir::Type& type) -> void {
            if (!type.is_unit()) {
                throw error(
                    std::format("The type of the loop body must be (), not {}", type),
                    body
                );
            }
        }

        auto operator()(ast::expression::Infinite_loop& loop) -> ir::Expression {
            auto body = context.resolve_expression(loop.body);
            ensure_loop_body_is_of_unit_type(loop.body, body.type);

            return {
                .value = ir::expression::Infinite_loop { std::move(body) },
                .type = ir::type::unit
            };
        }

        auto operator()(ast::expression::While_loop& loop) -> ir::Expression {
            auto condition = context.resolve_expression(loop.condition);
            if (!std::holds_alternative<ir::type::Boolean>(condition.type->value)) {
                throw error(
                    std::format(
                        "The condition must be of type Bool, not {}",
                        condition.type
                    ),
                    loop.condition
                );
            }

            auto body = context.resolve_expression(loop.body);
            ensure_loop_body_is_of_unit_type(loop.body, body.type);

            return {
                .value = ir::expression::While_loop {
                    std::move(condition),
                    std::move(body)
                },
                .type = ir::type::unit
            };
        }

        auto operator()(ast::expression::Compound& compound) -> ir::Expression {
            if (compound.expressions.empty()) {
                return {
                    .value = ir::expression::Tuple {},
                    .type  = ir::type::unit
                };
            }

            auto child_context = context.make_child_context_with_new_scope();

            std::vector<ir::Expression> side_effects;
            side_effects.reserve(compound.expressions.size() - 1);

            for (auto& expression : compound.expressions | std::views::take(compound.expressions.size() - 1)) {
                auto side_effect = child_context.resolve_expression(expression);

                if (!side_effect.type->is_unit()) {
                    throw error(
                        std::format(
                            "This expression is of type {}, but only the last expression "
                            "in a compound expression may be of a non-unit type",
                            side_effect.type
                        ),
                        std::format(
                            "If this is intentional, cast the expression to (), like this: {}{} as (){}",
                            bu::Color::dark_cyan,
                            expression,
                            bu::Color::white
                        ),
                        expression
                    );
                }

                side_effects.push_back(std::move(side_effect));
            }

            auto result = child_context.resolve_expression(compound.expressions.back());

            if (auto unused = child_context.scope.unused_variables()) {
                bu::abort("unused variables");
            }

            return {
                .value = ir::expression::Compound {
                    std::move(side_effects),
                    std::move(result)
                },
                .type = result.type
            };
        }

        auto operator()(auto&) -> ir::Expression {
            bu::abort(
                std::format(
                    "compiler::resolve_expression has not been implemented for {}",
                    this_expression
                )
            );
        }
    };

}


auto resolution::Resolution_context::resolve_expression(ast::Expression& expression) -> ir::Expression {
    return std::visit(Expression_resolution_visitor { *this, expression }, expression.value);
}