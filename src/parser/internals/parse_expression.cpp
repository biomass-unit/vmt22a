#include "bu/utilities.hpp"
#include "parser_internals.hpp"


namespace {

    using namespace parser;


    template <class T>
    auto extract_literal(Parse_context& context) -> ast::Expression {
        return ast::expression::Literal<T> { context.previous().value_as<T>() };
    }

    template <>
    auto extract_literal<char>(Parse_context& context) -> ast::Expression {
        return ast::expression::Literal { static_cast<bu::Char>(context.previous().as_character()) };
    }


    auto parse_struct_member_initializer(Parse_context& context)
        -> std::optional<bu::Pair<lexer::Identifier, bu::Wrapper<ast::Expression>>>
    {
        if (auto member = parse_lower_id(context)) {
            context.consume_required(Token::Type::equals);
            return bu::Pair { *member, bu::wrap(extract_expression(context)) };
        }
        else {
            return std::nullopt;
        }
    };

    auto extract_struct_initializer(ast::Type&& type, Parse_context& context)
        -> ast::Expression
    {
        static constexpr auto extract_member_initializers =
            extract_comma_separated_zero_or_more<parse_struct_member_initializer, "a member initializer">;

        auto initializers = extract_member_initializers(context);
        context.consume_required(Token::Type::brace_close);

        return ast::expression::Struct_initializer {
            .member_initializers { std::move(initializers) },
            .type                = std::move(type)
        };
    }


    auto extract_qualified_lower_name_or_struct_initializer(ast::Root_qualifier&& root, Parse_context& context)
        -> ast::Expression
    {
        auto* const anchor = context.pointer;
        auto name = extract_qualified(std::move(root), context);

        auto template_arguments = parse_template_arguments(context);

        if (!name.primary_qualifier.is_upper) {
            if (template_arguments) {
                return ast::expression::Template_instantiation {
                    std::move(*template_arguments),
                    std::move(name)
                };
            }
            else {
                return ast::expression::Variable { std::move(name) };
            }
        }
        else if (context.try_consume(Token::Type::brace_open)) {
            auto type = [&]() -> ast::Type {
                if (template_arguments) {
                    return ast::type::Template_instantiation {
                        std::move(*template_arguments),
                        std::move(name)
                    };
                }
                else {
                    return ast::type::Typename { std::move(name) };
                }
            }();

            // The source_view has to be assigned manually here because the
            // type hasn't gone through parse_type, which would normally do it.
            assign_source_view(type, anchor, context.pointer - 2);

            return extract_struct_initializer(std::move(type), context);
        }
        else {
            throw context.error(
                { anchor, context.pointer },
                "Expected an expression, but found a type"
            );
        }
    }


    auto extract_identifier(Parse_context& context) -> ast::Expression {
        context.retreat();
        return extract_qualified_lower_name_or_struct_initializer({ std::monostate {} }, context);
    }

    auto extract_global_identifier(Parse_context& context) -> ast::Expression {
        return extract_qualified_lower_name_or_struct_initializer({ ast::Root_qualifier::Global {} }, context);
    }

    auto extract_tuple(Parse_context& context) -> ast::Expression {
        auto expressions = extract_comma_separated_zero_or_more<parse_expression, "an expression">(context);
        context.consume_required(Token::Type::paren_close);
        if (expressions.size() == 1) {
            return std::move(expressions.front());
        }
        else {
            return ast::expression::Tuple { std::move(expressions) };
        }
    }

    auto extract_list_or_array(Parse_context& context) -> ast::Expression {
        // []     -> empty list
        // [;]    -> empty array
        // [1]    -> one-element list
        // [1;]   -> one-element array
        // [1, 2] -> two-element list
        // [1; 2] -> two-element array

        if (auto head = parse_expression(context)) {
            Token::Type delimiter;

            switch (auto& token = context.extract(); token.type) {
            case Token::Type::semicolon:
            case Token::Type::comma:
                delimiter = token.type;
                break;
            case Token::Type::bracket_close:
                return ast::expression::List_literal { { std::move(*head) } };
            default:
                context.retreat();
                throw context.expected("a ',', a ';', or a closing ']'");
            }

            auto elements = bu::vector_with_capacity<ast::Expression>(8);
            elements.push_back(std::move(*head));

            context.retreat();
            while (context.try_consume(delimiter)) {
                if (auto element = parse_expression(context)) {
                    elements.push_back(std::move(*element));
                }
                else if (delimiter == Token::Type::semicolon && elements.size() == 1) {
                    return context.try_consume(Token::Type::bracket_close)
                        ? ast::expression::Array_literal { std::move(elements) }
                        : throw context.expected("an expression or a closing ']'");
                }
                else {
                    throw context.expected("an expression");
                }
            }

            if (context.try_consume(Token::Type::bracket_close)) {
                if (delimiter == Token::Type::semicolon) {
                    return ast::expression::Array_literal { std::move(elements) };
                }
                else {
                    return ast::expression::List_literal { std::move(elements) };
                }
            }
            else {
                throw context.expected(std::format("a '{}' or a closing ']'", delimiter));
            }
        }
        else {
            bool const is_array = context.try_consume(Token::Type::semicolon);
            if (!context.try_consume(Token::Type::bracket_close)) {
                throw context.expected(
                    is_array
                        ? "a closing ']'"
                        : "an expression, a ';', or a closing ']'"
                );
            }
            if (is_array) {
                return ast::expression::Array_literal {};
            }
            else {
                return ast::expression::List_literal {};
            }
        }
    }

    auto extract_conditional(Parse_context& context) -> ast::Expression {
        constexpr std::string_view help =
            "the branches of a conditional expression must be compound expressions";

        if (auto condition = parse_expression(context)) {
            if (auto true_branch = parse_compound_expression(context)) {
                std::optional<bu::Wrapper<ast::Expression>> false_branch;

                if (context.try_consume(Token::Type::else_)) {
                    if (auto branch = parse_compound_expression(context)) {
                        false_branch.emplace(std::move(*branch));
                    }
                    else {
                        throw context.expected("the false branch", help);
                    }
                }
                else if (context.try_consume(Token::Type::elif)) {
                    false_branch.emplace(extract_conditional(context));
                }

                return ast::expression::Conditional {
                    std::move(*condition),
                    std::move(*true_branch),
                    std::move(false_branch)
                };
            }
            else {
                throw context.expected("the true branch", help);
            }
        }
        else {
            throw context.expected("a condition");
        }
    }

    auto extract_let_binding(Parse_context& context) -> ast::Expression {
        auto pattern = extract_pattern(context);
        
        std::optional<bu::Wrapper<ast::Type>> type;
        if (context.try_consume(Token::Type::colon)) {
            type.emplace(extract_type(context));
        }

        context.consume_required(Token::Type::equals);

        return ast::expression::Let_binding {
            std::move(pattern),
            extract_expression(context),
            std::move(type)
        };
    }

    auto extract_local_type_alias(Parse_context& context) -> ast::Expression {
        auto name = extract_upper_id(context, "an alias name");
        context.consume_required(Token::Type::equals);
        return ast::expression::Local_type_alias {
            std::move(name),
            extract_type(context)
        };
    }

    auto extract_loop_body(Parse_context& context) -> ast::Expression {
        if (auto body = parse_compound_expression(context)) {
            return std::move(*body);
        }
        else {
            throw context.expected("the loop body", "the loop body must be a compound expression");
        }
    }

    auto extract_infinite_loop(Parse_context& context) -> ast::Expression {
        return ast::expression::Infinite_loop { extract_loop_body(context) };
    }

    auto extract_while_loop(Parse_context& context) -> ast::Expression {
        auto condition = extract_expression(context);
        return ast::expression::While_loop {
            std::move(condition),
            extract_loop_body(context)
        };
    }

    auto extract_for_loop(Parse_context& context) -> ast::Expression {
        auto iterator = extract_pattern(context);
        context.consume_required(Token::Type::in);
        auto iterable = extract_expression(context);
        return ast::expression::For_loop {
            std::move(iterator),
            std::move(iterable),
            extract_loop_body(context)
        };
    }

    auto extract_size_of(Parse_context& context) -> ast::Expression {
        if (context.try_consume(Token::Type::paren_open)) {
            auto type = extract_type(context);
            context.consume_required(Token::Type::paren_close);
            return ast::expression::Size_of { std::move(type) };
        }
        else {
            throw context.expected("a parenthesized type");
        }
    }

    auto parse_match_case(Parse_context& context) -> std::optional<ast::expression::Match::Case> {
        if (auto pattern = parse_pattern(context)) {
            context.consume_required(Token::Type::right_arrow);
            return ast::expression::Match::Case {
                std::move(*pattern),
                extract_expression(context)
            };
        }
        else {
            return std::nullopt;
        }
    }

    auto extract_match(Parse_context& context) -> ast::Expression {
        constexpr auto parse_cases = braced<
            parse_separated_one_or_more<
                parse_match_case,
                Token::Type::semicolon,
                "a match case"
            >,
            "one or more match cases delimited by ';'"
        >;

        auto expression = extract_expression(context);

        if (auto cases = parse_cases(context)) {
            return ast::expression::Match {
                std::move(*cases),
                std::move(expression)
            };
        }
        else {
            throw context.expected("a '{' followed by match cases");
        }
    }

    auto extract_continue(Parse_context&) -> ast::Expression {
        return ast::expression::Continue {};
    }

    auto extract_break(Parse_context& context) -> ast::Expression {
        return ast::expression::Break {
            parse_expression(context).transform(bu::make<bu::Wrapper<ast::Expression>>)
        };
    }

    auto extract_ret(Parse_context& context) -> ast::Expression {
        return ast::expression::Ret {
            parse_expression(context).transform(bu::make<bu::Wrapper<ast::Expression>>)
        };
    }

    auto extract_take_reference(Parse_context& context) -> ast::Expression {
        auto mutability = extract_mutability(context);
        return ast::expression::Take_reference {
            .mutability = std::move(mutability),
            .name       = extract_lower_id(context, "a variable name")
        };
    }

    auto extract_meta(Parse_context& context) -> ast::Expression {
        if (context.try_consume(Token::Type::paren_open)) {
            auto expression = extract_expression(context);
            context.consume_required(Token::Type::paren_close);
            return ast::expression::Meta { std::move(expression) };
        }
        else {
            throw context.expected("a parenthesized expression");
        }
    }

    auto extract_compound_expression(Parse_context& context) -> ast::Expression {
        std::vector<ast::Expression> expressions;

        if (auto head = parse_expression(context)) {
            expressions.push_back(std::move(*head));

            while (context.try_consume(Token::Type::semicolon)) {
                if (auto expression = parse_expression(context)) {
                    expressions.push_back(std::move(*expression));
                }
                else {
                    expressions.push_back(ast::unit_value);
                    break;
                }
            }
        }

        context.consume_required(Token::Type::brace_close);

        return ast::expression::Compound { std::move(expressions) };
    }


    auto parse_normal_expression(Parse_context& context) -> std::optional<ast::Expression> {
        return parse_and_add_source_view<[](Parse_context& context) -> std::optional<ast::Expression> {
            switch (context.extract().type) {
            case Token::Type::integer:
                return extract_literal<bu::Isize>(context);
            case Token::Type::floating:
                return extract_literal<bu::Float>(context);
            case Token::Type::character:
                return extract_literal<char>(context);
            case Token::Type::boolean:
                return extract_literal<bool>(context);
            case Token::Type::string:
                return extract_literal<lexer::String>(context);
            case Token::Type::lower_name:
            case Token::Type::upper_name:
                return extract_identifier(context);
            case Token::Type::double_colon:
                return extract_global_identifier(context);
            case Token::Type::paren_open:
                return extract_tuple(context);
            case Token::Type::bracket_open:
                return extract_list_or_array(context);
            case Token::Type::if_:
                return extract_conditional(context);
            case Token::Type::let:
                return extract_let_binding(context);
            case Token::Type::alias:
                return extract_local_type_alias(context);
            case Token::Type::loop:
                return extract_infinite_loop(context);
            case Token::Type::while_:
                return extract_while_loop(context);
            case Token::Type::for_:
                return extract_for_loop(context);
            case Token::Type::size_of:
                return extract_size_of(context);
            case Token::Type::match:
                return extract_match(context);
            case Token::Type::continue_:
                return extract_continue(context);
            case Token::Type::break_:
                return extract_break(context);
            case Token::Type::ret:
                return extract_ret(context);
            case Token::Type::ampersand:
                return extract_take_reference(context);
            case Token::Type::meta:
                return extract_meta(context);
            case Token::Type::brace_open:
                return extract_compound_expression(context);
            default:
            {
                context.retreat();
                auto* const anchor = context.pointer;

                if (auto type = parse_type(context)) {
                    if (context.try_consume(Token::Type::double_colon)) {
                        return extract_qualified_lower_name_or_struct_initializer(ast::Root_qualifier { std::move(*type) }, context);
                    }
                    else if (context.try_consume(Token::Type::brace_open)) {
                        return extract_struct_initializer(std::move(*type), context);
                    }
                    else {
                        throw context.error(
                            { anchor, context.pointer },
                            "Expected an expression, but found a type"
                        );
                    }
                }
                else {
                    return std::nullopt;
                }
            }
            }
        }>(context);
    }


    auto parse_argument(Parse_context& context) -> std::optional<ast::Function_argument> {
        if (auto name = parse_lower_id(context)) {
            if (context.try_consume(Token::Type::equals)) {
                return ast::Function_argument { extract_expression(context), *name };
            }
            else {
                context.retreat();
            }
        }
        return parse_expression(context).transform(bu::make<ast::Function_argument>);
    }

    auto extract_arguments(Parse_context& context) -> std::vector<ast::Function_argument> {
        constexpr auto extract_arguments =
            extract_comma_separated_zero_or_more<parse_argument, "a function argument">;
        auto arguments = extract_arguments(context);

        context.consume_required(Token::Type::paren_close);
        return arguments;
    }


    auto parse_potential_invocation(Parse_context& context) -> std::optional<ast::Expression> {
        auto* const anchor = context.pointer;

        if (auto potential_invocable = parse_normal_expression(context)) {
            while (context.try_consume(Token::Type::paren_open)) {
                *potential_invocable = ast::expression::Invocation {
                    extract_arguments(context),
                    std::move(*potential_invocable)
                };
                assign_source_view(*potential_invocable, anchor, context.pointer - 1);
            }
            return potential_invocable;
        }
        else {
            return std::nullopt;
        }
    }


    auto parse_potential_member_access(Parse_context& context) -> std::optional<ast::Expression> {
        auto expression = parse_potential_invocation(context);

        if (expression) {
            std::vector<ast::expression::Member_access_chain::Accessor> accessors;

            while (context.try_consume(Token::Type::dot)) {
                if (auto member_name = parse_lower_id(context)) {
                    if (context.try_consume(Token::Type::paren_open)) {
                        if (!accessors.empty()) {
                            *expression = ast::expression::Member_access_chain {
                                std::move(accessors),
                                std::move(*expression)
                            };
                            accessors = {}; // Silence warnings, even though a moved-from vector is guaranteed to be empty
                        }
                        *expression = ast::expression::Member_function_invocation {
                            extract_arguments(context),
                            std::move(*expression),
                            *member_name
                        };
                    }
                    else {
                        accessors.emplace_back(*member_name);
                    }
                }
                else if (auto member_index = context.try_extract(Token::Type::integer)) {
                    accessors.emplace_back(member_index->as_integer());
                }
                else {
                    throw context.expected("a member name or index");
                }
            }

            if (!accessors.empty()) {
                return ast::expression::Member_access_chain {
                    std::move(accessors),
                    std::move(*expression)
                };
            }
        }

        return expression;
    }


    auto parse_potential_type_cast(Parse_context& context) -> std::optional<ast::Expression> {
        if (auto expression = parse_potential_member_access(context)) {
            while (context.try_consume(Token::Type::as)) {
                *expression = ast::expression::Type_cast {
                    std::move(*expression),
                    extract_type(context)
                };
            }
            return expression;
        }
        else {
            return std::nullopt;
        }
    }


    template <bu::Metastring... strings>
    std::array id_array {
        lexer::Identifier { strings.view(), lexer::Identifier::guaranteed_new_string }...
    };

    std::tuple precedence_table {
        id_array<"*", "/", "%">,
        id_array<"+", "-">,
        id_array<"?=", "!=">,
        id_array<"<", "<=", ">=", ">">,
        id_array<"&&", "||">,
        id_array<":=", "+=", "*=", "/=", "%=">
    };

    constexpr auto lowest_precedence = std::tuple_size_v<decltype(precedence_table)> - 1;


    auto parse_operator(Parse_context& context) -> std::optional<lexer::Identifier> {
        static std::optional const asterisk = [] {
            auto const identifier = std::get<0>(precedence_table).front();
            assert(identifier.view() == "*");
            return identifier;
        }();
        static std::optional const plus = [] {
            auto const identifier = std::get<1>(precedence_table).front();
            assert(identifier.view() == "+");
            return identifier;
        }();

        switch (context.extract().type) {
        case Token::Type::operator_name:
            return context.previous().as_identifier();
        case Token::Type::asterisk:
            return asterisk;
        case Token::Type::plus:
            return plus;
        default:
            context.retreat();
            return std::nullopt;
        }
    }

    template <bu::Usize precedence>
    auto parse_binary_operator_invocation_with_precedence(Parse_context& context)
        -> std::optional<ast::Expression>
    {
        constexpr auto recurse =
            parse_binary_operator_invocation_with_precedence<precedence - 1>;

        if (auto left = recurse(context)) {
            while (auto op = parse_operator(context)) {
                if constexpr (precedence != lowest_precedence) {
                    constexpr auto& ops = std::get<precedence>(precedence_table);
                    if (std::ranges::find(ops, op) == ops.end()) {
                        context.retreat(); // not in current operator group
                        break;
                    }
                }
                if (auto right = recurse(context)) {
                    *left = ast::expression::Binary_operator_invocation {
                        std::move(*left),
                        std::move(*right),
                        *op
                    };
                }
                else {
                    throw context.expected("an operand");
                }
            }
            return left;
        }
        else {
            return std::nullopt;
        }
    }

    template <>
    auto parse_binary_operator_invocation_with_precedence<static_cast<bu::Usize>(-1)>(Parse_context& context)
        -> std::optional<ast::Expression>
    {
        return parse_potential_type_cast(context);
    }

}


auto parser::parse_expression(Parse_context& context) -> std::optional<ast::Expression> {
    return parse_and_add_source_view<
        parse_binary_operator_invocation_with_precedence<lowest_precedence>
    >(context);
}

auto parser::parse_compound_expression(Parse_context& context) -> std::optional<ast::Expression> {
    return parse_and_add_source_view<
        [](Parse_context& context) -> std::optional<ast::Expression> {
            if (context.try_consume(Token::Type::brace_open)) {
                return extract_compound_expression(context);
            }
            else {
                return std::nullopt;
            }
        }
    >(context);
}