#include "bu/utilities.hpp"
#include "bu/uninitialized.hpp"
#include "parser.hpp"
#include "internals/parser_internals.hpp"


auto parser::parse_template_arguments(Parse_context& context)
    -> std::optional<std::vector<ast::Template_argument>>
{
    constexpr auto extract_arguments = extract_comma_separated_zero_or_more<[](Parse_context& context)
        -> std::optional<ast::Template_argument>
    {
        if (auto expression = parse_expression(context)) {
            return ast::Template_argument { std::move(*expression) };
        }
        else if (auto type = parse_type(context)) {
            return ast::Template_argument { std::move(*type) };
        }
        else {
            return std::nullopt;
        }
    }, "a template argument">;

    if (context.try_consume(Token::Type::bracket_open)) {
        auto arguments = extract_arguments(context);
        context.consume_required(Token::Type::bracket_close);
        return std::move(arguments);
    }
    else {
        return std::nullopt;
    }
}


namespace {

    using namespace parser;


    thread_local ast::Namespace* current_namespace;


    auto parse_class_reference(Parse_context&) -> std::optional<ast::Class_reference>;

    auto extract_classes(Parse_context& context) -> std::vector<ast::Class_reference> {
        constexpr auto extract_classes =
            extract_comma_separated_zero_or_more<parse_class_reference, "a class name">;

        auto classes = extract_classes(context);
        if (classes.empty()) {
            throw context.expected("one or more class names");
        }
        else {
            return classes;
        }
    }

    auto parse_template_parameters(Parse_context& context)
        -> std::optional<std::vector<ast::Template_parameter>>
    {
        constexpr auto extract_parameters = parse_comma_separated_one_or_more<[](Parse_context& context)
            -> std::optional<ast::Template_parameter>
        {
            if (auto name = parse_lower_id(context)) {
                context.consume_required(Token::Type::colon);
                return ast::Template_parameter {
                    ast::Template_parameter::Value_parameter {
                        *name,
                        extract_type(context)
                    }
                };
            }
            else if (auto name = parse_upper_id(context)) {
                std::vector<ast::Class_reference> classes;
                if (context.try_consume(Token::Type::colon)) {
                    classes = extract_classes(context);
                }

                return ast::Template_parameter {
                    ast::Template_parameter::Type_parameter {
                        std::move(classes),
                        *name
                    }
                };
            }
            else {
                return std::nullopt;
            }
        }, "a template parameter">;

        if (context.try_consume(Token::Type::bracket_open)) {
            if (auto parameters = extract_parameters(context)) {
                context.consume_required(Token::Type::bracket_close);
                return parameters;
            }
            else {
                throw context.expected("one or more template parameters");
            }
        }
        else {
            return std::nullopt;
        }
    }


    auto parse_function_parameter(Parse_context& context)
        -> std::optional<ast::definition::Function::Parameter>
    {
        if (auto pattern = parse_pattern(context)) {
            context.consume_required(Token::Type::colon);
            auto return_type = extract_type(context);

            std::optional<bu::Wrapper<ast::Expression>> default_value;
            if (context.try_consume(Token::Type::equals)) {
                default_value.emplace(extract_expression(context));
            }

            return ast::definition::Function::Parameter {
                std::move(*pattern),
                std::move(return_type),
                std::move(default_value)
            };
        }
        else {
            return std::nullopt;
        }
    }

    auto extract_function(Parse_context& context) -> void {
        auto name = extract_lower_id<"a function name">(context);
        auto template_parameters = parse_template_parameters(context);

        if (context.try_consume(Token::Type::paren_open)) {
            constexpr auto parse_parameters =
                extract_comma_separated_zero_or_more<parse_function_parameter, "a function parameter">;
            auto parameters = parse_parameters(context);
            context.consume_required(Token::Type::paren_close);

            std::optional<bu::Wrapper<ast::Type>> return_type;
            if (context.try_consume(Token::Type::colon)) {
                return_type.emplace(extract_type(context));
            }

            /*if (context.try_consume(Token::Type::where)) {

            }*/

            bu::Uninitialized<ast::Expression> body;

            if (auto expression = parse_compound_expression(context)) {
                body.initialize(std::move(*expression));
            }
            else if (context.try_consume(Token::Type::equals)) {
                body.initialize(extract_expression(context));
            }
            else {
                throw context.expected("the function body", "'=' or '{'");
            }

            current_namespace->function_definitions.emplace_back(
                std::move(*body),
                std::move(parameters),
                std::move(template_parameters),
                name,
                return_type
            );
        }
        else {
            throw context.expected("a parenthesized list of function parameters");
        }
    }


    auto parse_struct_member(Parse_context& context)
        -> std::optional<ast::definition::Struct::Member>
    {
        if (auto name = parse_lower_id(context)) {
            context.consume_required(Token::Type::colon);
            return ast::definition::Struct::Member {
                *name,
                extract_type(context)
            };
        }
        else {
            return std::nullopt;
        }
    }

    auto extract_struct(Parse_context& context) -> void {
        constexpr auto parse_members =
            parse_comma_separated_one_or_more<parse_struct_member, "a struct member">;

        auto name = extract_upper_id<"a struct name">(context);
        auto template_parameters = parse_template_parameters(context);

        context.consume_required(Token::Type::equals);
        if (auto members = parse_members(context)) {
            current_namespace->struct_definitions.emplace_back(
                std::move(template_parameters),
                std::move(*members),
                name
            );
        }
        else {
            throw context.expected("one or more struct members");
        }
    }


    auto parse_data_constructor(Parse_context& context)
        -> std::optional<ast::definition::Data::Constructor>
    {
        if (auto name = parse_upper_id(context)) {
            std::optional<bu::Wrapper<ast::Type>> type;

            if (context.try_consume(Token::Type::paren_open)) {
                auto types = extract_comma_separated_zero_or_more<parse_type, "a type">(context);
                switch (types.size()) {
                case 0:
                    break;
                case 1:
                    type.emplace(std::move(types.front()));
                    break;
                default:
                    type.emplace(ast::type::Tuple { std::move(types) });
                }
                context.consume_required(Token::Type::paren_close);
            }

            return ast::definition::Data::Constructor { *name, type };
        }
        else {
            return std::nullopt;
        }
    }

    auto extract_data(Parse_context& context) -> void {
        constexpr auto parse_constructors =
            parse_separated_one_or_more<parse_data_constructor, Token::Type::pipe, "a data constructor">;

        auto name = extract_upper_id<"a data name">(context);
        auto template_parameters = parse_template_parameters(context);

        context.consume_required(Token::Type::equals);
        if (auto constructors = parse_constructors(context)) {
            current_namespace->data_definitions.emplace_back(
                std::move(template_parameters),
                std::move(*constructors),
                name
            );
        }
        else {
            throw context.expected("one or more data constructors");
        }
    }


    auto parse_class_reference(Parse_context& context)
        -> std::optional<ast::Class_reference>
    {
        auto name = extract_upper_id<"a class name">(context);
        return ast::Class_reference {
            parse_template_arguments(context),
            name
        };
    }

    auto extract_function_signature(Parse_context& context) -> ast::Function_signature {
        auto name = extract_lower_id<"a function name">(context);
        auto template_parameters = parse_template_parameters(context);

        context.consume_required(Token::Type::paren_open);
        auto parameters = extract_comma_separated_zero_or_more<parse_wrapped<parse_type>, "a parameter type">(context);
        context.consume_required(Token::Type::paren_close);

        context.consume_required(Token::Type::colon);
        return ast::Function_signature {
            std::move(template_parameters),
            {
                .argument_types = std::move(parameters),
                .return_type    = extract_type(context)
            },
            name
        };
    }

    auto extract_type_signature(Parse_context& context) -> ast::Type_signature {
        auto name = extract_upper_id<"an alias name">(context);

        auto template_parameters = parse_template_parameters(context);

        std::vector<ast::Class_reference> classes;
        if (context.try_consume(Token::Type::colon)) {
            classes = extract_classes(context);
        }

        return ast::Type_signature {
            std::move(template_parameters),
            std::move(classes),
            name
        };
    }

    auto extract_class(Parse_context& context) -> void {
        auto name = extract_upper_id<"a class name">(context);

        bool self_is_template = false;
        if (context.try_consume(Token::Type::bracket_open)) {
            self_is_template = true;
            context.consume_required(Token::Type::bracket_close);
        }

        std::vector<ast::Type_signature>     type_signatures;
        std::vector<ast::Function_signature> function_signatures;

        bool is_braced = context.try_consume(Token::Type::brace_open);
        if (!is_braced) {
            context.consume_required(Token::Type::equals);
        }

        for (;;) {
            switch (context.extract().type) {
            case Token::Type::fn:
                function_signatures.push_back(extract_function_signature(context));
                continue;
            case Token::Type::alias:
                type_signatures.push_back(extract_type_signature(context));
                continue;
            default:
                context.retreat();
                if (function_signatures.empty() && type_signatures.empty()) {
                    throw context.expected("one or more function or type signatures");
                }
                if (is_braced) {
                    context.consume_required(Token::Type::brace_close);
                }
                current_namespace->class_definitions.emplace_back(
                    std::move(function_signatures),
                    std::move(type_signatures),
                    name,
                    self_is_template
                );
                return;
            }
        }
    }


    auto parse_definition(Parse_context&) -> bool;

    auto extract_namespace(Parse_context& context) -> void {
        auto name = extract_lower_id<"a module name">(context);

        context.consume_required(Token::Type::brace_open);

        auto parent_namespace = current_namespace;

        current_namespace =  parent_namespace->make_child(name);
        while (parse_definition(context));
        current_namespace = parent_namespace;

        context.consume_required(Token::Type::brace_close);
    }


    auto parse_definition(Parse_context& context) -> bool {
        switch (context.extract().type) {
        case Token::Type::fn:
            extract_function(context);
            break;
        case Token::Type::struct_:
            extract_struct(context);
            break;
        case Token::Type::data:
            extract_data(context);
            break;
        case Token::Type::class_:
            extract_class(context);
            break;
        case Token::Type::module:
            extract_namespace(context);
            break;
        default:
            context.retreat();
            return false;
        }
        return true;
    }

}


auto parser::parse(lexer::Tokenized_source&& tokenized_source) -> ast::Module {
    Parse_context context { tokenized_source };
    ast::Namespace global_namespace { lexer::Identifier { std::string_view { "" } } };
    ::current_namespace = &global_namespace;

    while (parse_definition(context));

    if (!context.is_finished()) {
        throw context.expected("a definition");
    }

    return { std::move(tokenized_source.source), global_namespace };
}