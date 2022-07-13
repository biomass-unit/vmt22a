#include "bu/utilities.hpp"
#include "parser_internals.hpp"
#include "parser.hpp"


auto parser::parse_template_arguments(Parse_context& context)
    -> std::optional<std::vector<ast::Template_argument>>
{
    static constexpr auto extract_arguments = extract_comma_separated_zero_or_more<[](Parse_context& context)
        -> std::optional<ast::Template_argument>
    {
        if (auto type = parse_type(context)) {
            return ast::Template_argument { bu::wrap(std::move(*type)) };
        }
        else if (auto expression = parse_expression(context)) {
            return ast::Template_argument { bu::wrap(std::move(*expression)) };
        }
        else {
            std::optional<ast::Mutability> mutability;

            auto* const anchor = context.pointer;

            auto const get_source_view = [&] {
                return make_source_view(anchor, context.pointer - 1);
            };

            if (context.try_consume(Token::Type::mut)) {
                if (context.try_consume(Token::Type::question)) {
                    mutability.emplace(
                        ast::Mutability {
                            .parameter_name = extract_lower_id(context, "a mutability parameter name"),
                            .type           = ast::Mutability::Type::parameterized,
                            .source_view    = get_source_view()
                        }
                    );
                }
                else {
                    mutability.emplace(
                        ast::Mutability {
                            .type        = ast::Mutability::Type::mut,
                            .source_view = get_source_view()
                        }
                    );
                }
            }
            else if (context.try_consume(Token::Type::immut)) {
                mutability.emplace(
                    ast::Mutability {
                        .type        = ast::Mutability::Type::immut,
                        .source_view = get_source_view()
                    }
                );
            }

            return mutability.transform(bu::make<ast::Template_argument>);
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

auto parser::extract_function_parameters(Parse_context& context)
    -> std::vector<ast::Function_parameter>
{
    return extract_comma_separated_zero_or_more<
        [](Parse_context& context)
            -> std::optional<ast::Function_parameter>
        {
            if (auto pattern = parse_pattern(context)) {
                std::optional<bu::Wrapper<ast::Type>> type;
                if (context.try_consume(Token::Type::colon)) {
                    type = extract_type(context);
                }

                std::optional<bu::Wrapper<ast::Expression>> default_value;
                if (context.try_consume(Token::Type::equals)) {
                    default_value = extract_expression(context);
                }

                return ast::Function_parameter {
                    std::move(*pattern),
                    std::move(type),
                    std::move(default_value)
                };
            }
            else {
                return std::nullopt;
            }
        },
        "a function parameter"
    >(context);
}

auto parser::extract_qualified(ast::Root_qualifier&& root, Parse_context& context)
    -> ast::Qualified_name
{
    std::vector<ast::Qualifier> qualifiers;
    lexer::Token* template_argument_anchor = nullptr;

    auto extract_qualifier = [&]() -> bool {
        switch (auto& token = context.extract(); token.type) {
        case Token::Type::lower_name:
        case Token::Type::upper_name:
        {
            template_argument_anchor = context.pointer;
            auto template_arguments = parse_template_arguments(context);

            ast::Qualifier qualifier {
                .template_arguments = std::move(template_arguments),
                .name {
                    .identifier  = token.as_identifier(),
                    .is_upper    = token.type == Token::Type::upper_name,
                    .source_view = template_argument_anchor[-1].source_view
                },
                .source_view = make_source_view(template_argument_anchor - 1, context.pointer - 1)
            };

            qualifiers.push_back(std::move(qualifier));
            return true;
        }
        default:
            context.retreat();
            return false;
        }
    };

    if (extract_qualifier()) {
        while (context.try_consume(Token::Type::double_colon)) {
            if (!extract_qualifier()) {
                context.error_expected("an identifier");
            }
        }

        auto back = std::move(qualifiers.back());
        qualifiers.pop_back();

        // Ignore potential template arguments, they are handled separately
        context.pointer = template_argument_anchor;

        return {
            .middle_qualifiers = std::move(qualifiers),
            .root_qualifier    = std::move(root),
            .primary_name      = std::move(back.name)
        };
    }
    else {
        // root:: followed by no qualifiers
        context.error_expected("an identifier");
    }
}

auto parser::extract_mutability(Parse_context& context) -> ast::Mutability {
    auto* const anchor = context.pointer;

    auto const get_source_view = [&] {
        return context.pointer == anchor
            ? anchor->source_view
            : make_source_view(anchor, context.pointer - 1);
    };

    if (context.try_consume(Token::Type::mut)) {
        if (context.try_consume(Token::Type::question)) {
            return {
                .parameter_name = extract_lower_id(context, "a mutability parameter name"),
                .type           = ast::Mutability::Type::parameterized,
                .source_view    = get_source_view()
            };
        }
        else {
            return {
                .type        = ast::Mutability::Type::mut,
                .source_view = get_source_view()
            };
        }
    }
    else {
        return {
            .type        = ast::Mutability::Type::immut,
            .source_view = get_source_view()
        };
    }
}


namespace {
    auto parse_class_reference(parser::Parse_context&) -> std::optional<ast::Class_reference>;
}

auto parser::extract_class_references(Parse_context& context)
    -> std::vector<ast::Class_reference>
{
    static constexpr auto extract_classes =
        extract_separated_zero_or_more<parse_class_reference, Token::Type::plus, "a class name">;

    auto classes = extract_classes(context);

    if (classes.empty()) {
        context.error_expected("one or more class names");
    }
    else {
        return classes;
    }
}


namespace {

    using namespace parser;


    auto parse_definition(Parse_context&) -> std::optional<ast::Definition>;

    auto extract_definition_sequence(Parse_context& context) -> std::vector<ast::Definition> {
        std::vector<ast::Definition> definitions;
        definitions.reserve(16);

        while (auto definition = parse_definition(context)) {
            definitions.push_back(std::move(*definition));
        }

        return definitions;
    }

    auto extract_braced_definition_sequence(Parse_context& context) -> std::vector<ast::Definition> {
        context.consume_required(Token::Type::brace_open);
        auto definitions = extract_definition_sequence(context);
        context.consume_required(Token::Type::brace_close);
        return definitions;
    }


    auto parse_class_reference(Parse_context&) -> std::optional<ast::Class_reference>;


    auto parse_template_parameters(Parse_context& context)
        -> std::optional<std::vector<ast::Template_parameter>>
    {
        constexpr auto extract_parameters = parse_comma_separated_one_or_more<[](Parse_context& context)
            -> std::optional<ast::Template_parameter>
        {
            auto const get_source_view = [&, anchor = context.pointer] {
                return make_source_view(anchor, context.pointer - 1);
            };

            if (auto name = parse_lower_name(context)) {
                if (context.try_consume(Token::Type::colon)) {
                    if (context.try_consume(Token::Type::mut)) {
                        return ast::Template_parameter {
                            .value       = ast::Template_parameter::Mutability_parameter {},
                            .name        = std::move(*name),
                            .source_view = get_source_view()
                        };
                    }
                    else if (auto type = parse_type(context)) {
                        return ast::Template_parameter {
                            .value       = ast::Template_parameter::Value_parameter { std::move(*type) },
                            .name        = std::move(*name),
                            .source_view = get_source_view()
                        };
                    }
                    else {
                        context.error_expected("'mut' or a type");
                    }
                }

                return ast::Template_parameter {
                    .value       = ast::Template_parameter::Value_parameter { .type = std::nullopt },
                    .name        = std::move(*name),
                    .source_view = get_source_view()
                };
            }
            else if (auto name = parse_upper_name(context)) {
                std::vector<ast::Class_reference> classes;
                if (context.try_consume(Token::Type::colon)) {
                    classes = extract_class_references(context);
                }

                return ast::Template_parameter {
                    .value       = ast::Template_parameter::Type_parameter { std::move(classes) },
                    .name        = std::move(*name),
                    .source_view = get_source_view()
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
                context.error_expected("one or more template parameters");
            }
        }
        else {
            return std::nullopt;
        }
    }


    constexpr Extractor extract_function = +[](Parse_context& context)
        -> ast::Definition::Variant
    {
        auto name                = extract_lower_name(context, "a function name");
        auto template_parameters = parse_template_parameters(context);

        if (context.try_consume(Token::Type::paren_open)) {
            auto parameters = extract_function_parameters(context);
            context.consume_required(Token::Type::paren_close);

            std::optional<bu::Wrapper<ast::Type>> return_type;
            if (context.try_consume(Token::Type::colon)) {
                return_type.emplace(extract_type(context));
            }

            if (context.try_consume(Token::Type::where)) {
                bu::todo(); // TODO: add support for where clauses
            }

            auto body = [&] {
                if (auto expression = parse_block_expression(context)) {
                    return std::move(*expression);
                }
                else if (context.try_consume(Token::Type::equals)) {
                    return extract_expression(context);
                }
                else {
                    context.error_expected("the function body", "'=' or '{'");
                }
            }();

            return ast::definition::Function {
                std::move(body),
                std::move(parameters),
                std::move(name),
                return_type,
                std::move(template_parameters)
            };
        }
        else {
            context.error_expected("a parenthesized list of function parameters");
        }
    };


    template <class Member>
    auto ensure_no_duplicate_members(
        Parse_context            & context,
        std::vector<Member> const& range,
        std::string_view    const  description
    )
        -> void
    {
        for (auto it = range.cbegin(); it != range.cend(); ++it) {
            auto found = std::ranges::find(range.cbegin(), it, it->name, &Member::name);

            if (found != it) {
                context.error(it->source_view, {
                    .message_format = "A {} with this name has already been defined",
                    .message_arguments = std::make_format_args(description)
                });

                // TODO: add more info to the error message
            }
        }
    }


    auto parse_struct_member(Parse_context& context)
        -> std::optional<ast::definition::Struct::Member>
    {
        auto* const anchor    = context.pointer;
        bool  const is_public = context.try_consume(Token::Type::pub);

        if (auto name = parse_lower_id(context)) {
            context.consume_required(Token::Type::colon);

            auto type = extract_type(context);

            return ast::definition::Struct::Member {
                .name        = *name,
                .type        = std::move(type),
                .is_public   = is_public,
                .source_view = make_source_view(anchor, context.pointer - 1)
            };
        }
        else if (is_public) {
            context.error_expected("a struct member name");
        }
        else {
            return std::nullopt;
        }
    }

    constexpr Extractor extract_struct = +[](Parse_context& context)
        -> ast::Definition::Variant
    {
        static constexpr auto parse_members =
            parse_comma_separated_one_or_more<parse_struct_member, "a struct member">;

        auto name                = extract_upper_name(context, "a struct name");
        auto template_parameters = parse_template_parameters(context);

        context.consume_required(Token::Type::equals);

        if (auto members = parse_members(context)) {
            ensure_no_duplicate_members(context, *members, "member");

            return ast::definition::Struct {
                std::move(*members),
                std::move(name),
                std::move(template_parameters)
            };
        }
        else {
            context.error_expected("one or more struct members");
        }
    };


    auto parse_enum_constructor(Parse_context& context)
        -> std::optional<ast::definition::Enum::Constructor>
    {
        auto* const anchor = context.pointer;

        if (auto name = parse_lower_id(context)) {
            std::optional<bu::Wrapper<ast::Type>> type;

            if (context.try_consume(Token::Type::paren_open)) {
                auto types = extract_comma_separated_zero_or_more<parse_type, "a type">(context);

                switch (types.size()) {
                case 0:
                    break;
                case 1:
                    type = std::move(types.front());
                    break;
                default:
                    type = ast::Type {
                        .value       = ast::type::Tuple { std::move(types) },
                        .source_view = types.front().source_view + types.back().source_view
                    };
                }

                context.consume_required(Token::Type::paren_close);
            }

            return ast::definition::Enum::Constructor {
                .name        = *name,
                .type        = type,
                .source_view = make_source_view(anchor, context.pointer - 1)
            };
        }
        else {
            return std::nullopt;
        }
    }

    constexpr Extractor extract_enum = +[](Parse_context& context)
        -> ast::Definition::Variant
    {
        static constexpr auto parse_constructors =
            parse_separated_one_or_more<parse_enum_constructor, Token::Type::pipe, "an enum constructor">;

        auto* anchor = context.pointer;

        auto name                = extract_upper_name(context, "an enum name");
        auto template_parameters = parse_template_parameters(context);

        context.consume_required(Token::Type::equals);
        if (auto constructors = parse_constructors(context)) {
            ensure_no_duplicate_members(context, *constructors, "constructor");

            static constexpr auto max =
                std::numeric_limits<bu::U8>::max();

            if (constructors->size() > max) {
                // This allows the tag to always be a single byte
                context.error(
                    { anchor - 1, anchor + 1 },
                    {
                        .message_format =
                            "An enum-definition must not define more "
                            "than {} constructors, but {} defines {}",
                        .message_arguments = std::make_format_args(max, name, constructors->size()),
                        .help_note =
                            "If this is truly necessary, consider categorizing "
                            "the constructors under several simpler types"
                    }
                );
            }

            return ast::definition::Enum {
                std::move(*constructors),
                std::move(name),
                std::move(template_parameters)
            };
        }
        else {
            context.error_expected("one or more enum constructors");
        }
    };


    constexpr Extractor extract_alias = +[](Parse_context& context)
        -> ast::Definition::Variant
    {
        auto name                = extract_upper_name(context, "an alias name");
        auto template_parameters = parse_template_parameters(context);

        context.consume_required(Token::Type::equals);

        return ast::definition::Alias {
            name,
            extract_type(context),
            std::move(template_parameters)
        };
    };


    auto parse_class_reference(Parse_context& context)
        -> std::optional<ast::Class_reference>
    {
        auto* const anchor = context.pointer;

        auto name = [&]() -> std::optional<ast::Qualified_name> {
            ast::Root_qualifier root;
            auto* const anchor = context.pointer;

            if (context.try_consume(Token::Type::upper_name) || context.try_consume(Token::Type::lower_name)) {
                context.retreat();
            }
            else if (context.try_consume(Token::Type::double_colon)) {
                root.value = ast::Root_qualifier::Global{};
            }
            else {
                return std::nullopt;
            }

            auto name = extract_qualified(std::move(root), context);

            if (name.primary_name.is_upper) {
                return name;
            }
            else {
                context.error(
                    { anchor, context.pointer },
                    "Expected a class name, but found a lowercase identifier"
                );
            }
        }();

        if (name) {
            auto template_arguments = parse_template_arguments(context);

            return ast::Class_reference {
                .template_arguments = std::move(template_arguments),
                .name               = std::move(*name),
                .source_view        = make_source_view(anchor, context.pointer - 1)
            };
        }
        else {
            return std::nullopt;
        }
    }


    constexpr Extractor extract_implementation = +[](Parse_context& context)
        -> ast::Definition::Variant
    {
        auto template_parameters = parse_template_parameters(context);
        auto type                = extract_type(context);
        auto definitions         = extract_braced_definition_sequence(context);

        return ast::definition::Implementation {
            std::move(type),
            std::move(definitions),
            std::move(template_parameters)
        };
    };


    constexpr Extractor extract_instantiation = +[](Parse_context& context)
        -> ast::Definition::Variant
    {
        auto template_parameters = parse_template_parameters(context);

        if (auto typeclass = parse_class_reference(context)) {
            auto type        = extract_type(context);
            auto definitions = extract_braced_definition_sequence(context);

            return ast::definition::Instantiation {
                .typeclass           = std::move(*typeclass),
                .instance            = std::move(type),
                .definitions         = std::move(definitions),
                .template_parameters = std::move(template_parameters)
            };
        }
        else {
            context.error_expected("a class name");
        }
    };


    auto extract_function_signature(Parse_context& context) -> ast::Function_signature {
        auto name                = extract_lower_name(context, "a function name");
        auto template_parameters = parse_template_parameters(context);

        context.consume_required(Token::Type::paren_open);
        auto parameters = extract_comma_separated_zero_or_more<parse_type, "a parameter type">(context);
        context.consume_required(Token::Type::paren_close);

        context.consume_required(Token::Type::colon);
        return ast::Function_signature {
            std::move(template_parameters),
            {
                .argument_types = std::move(parameters),
                .return_type    = extract_type(context)
            },
            std::move(name)
        };
    }

    auto extract_type_signature(Parse_context& context) -> ast::Type_signature {
        auto name = extract_upper_name(context, "an alias name");

        auto template_parameters = parse_template_parameters(context);

        std::vector<ast::Class_reference> classes;
        if (context.try_consume(Token::Type::colon)) {
            classes = extract_class_references(context);
        }

        return ast::Type_signature {
            std::move(template_parameters),
            std::move(classes),
            std::move(name)
        };
    }

    constexpr Extractor extract_class = +[](Parse_context& context)
        -> ast::Definition::Variant
    {
        auto name = extract_upper_name(context, "a class name");

        auto template_parameters = parse_template_parameters(context);

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
                    context.error_expected("one or more function or type signatures");
                }
                if (is_braced) {
                    context.consume_required(Token::Type::brace_close);
                }

                return ast::definition::Typeclass {
                    std::move(function_signatures),
                    std::move(type_signatures),
                    std::move(name),
                    std::move(template_parameters)
                };
            }
        }
    };


    constexpr Extractor extract_namespace = +[](Parse_context& context)
        -> ast::Definition::Variant
    {
        auto name                = extract_lower_name(context, "a namespace name");
        auto template_parameters = parse_template_parameters(context);

        return ast::definition::Namespace {
            extract_braced_definition_sequence(context),
            std::move(name),
            std::move(template_parameters)
        };
    };


    auto parse_definition(Parse_context& context) -> std::optional<ast::Definition> {
        switch (context.extract().type) {
        case Token::Type::fn:
            return extract_function(context);
        case Token::Type::struct_:
            return extract_struct(context);
        case Token::Type::enum_:
            return extract_enum(context);
        case Token::Type::alias:
            return extract_alias(context);
        case Token::Type::class_:
            return extract_class(context);
        case Token::Type::impl:
            return extract_implementation(context);
        case Token::Type::inst:
            return extract_instantiation(context);
        case Token::Type::namespace_:
            return extract_namespace(context);
        default:
            context.retreat();
            return std::nullopt;
        }
    }

}


auto parser::parse(lexer::Tokenized_source&& tokenized_source) -> ast::Module {
    Parse_context context { tokenized_source };

    ast::Node_context                module_context;
    std::vector<ast::Import>         module_imports;
    std::optional<lexer::Identifier> module_name;

    if (context.try_consume(Token::Type::module_)) {
        module_name = extract_lower_id(context, "a module name");
    }

    while (context.try_consume(Token::Type::import_)) {
        static constexpr auto parse_path =
            parse_separated_one_or_more<parse_lower_id, Token::Type::dot, "a module qualifier">;

        if (auto path = parse_path(context)) {
            lexer::Identifier module_name = path->back();
            path->pop_back();

            ast::Import import_statement { .path { std::move(*path), module_name } };

            if (context.try_consume(Token::Type::as)) {
                import_statement.alias = extract_lower_id(context, "a module alias");
            }

            module_imports.push_back(std::move(import_statement));
        }
        else {
            context.error_expected("a module path");
        }
    }

    auto definitions = extract_definition_sequence(context);

    if (!context.is_finished()) {
        context.error_expected(
            "a definition",
            "'fn', 'struct', 'enum', 'alias', 'impl', 'inst', or 'class'"
        );
    }

    return ast::Module {
        .node_context = std::move(module_context),
        .diagnostics  = std::move(tokenized_source.diagnostics),
        .source       = std::move(tokenized_source.source),
        .definitions  = std::move(definitions),
        .name         = std::move(module_name),
        .imports      = std::move(module_imports)
    };
}