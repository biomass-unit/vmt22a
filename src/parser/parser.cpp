#include "bu/utilities.hpp"
#include "internals/parser_internals.hpp"
#include "parser.hpp"


auto parser::parse_template_arguments(Parse_context& context)
    -> std::optional<std::vector<ast::Template_argument>>
{
    constexpr auto extract_arguments = extract_comma_separated_zero_or_more<[](Parse_context& context)
        -> std::optional<ast::Template_argument>
    {
        if (auto type = parse_type(context)) {
            return ast::Template_argument { std::move(*type) };
        }
        else if (auto expression = parse_expression(context)) {
            return ast::Template_argument { std::move(*expression) };
        }
        else {
            std::optional<ast::Mutability> mutability;

            if (context.try_consume(Token::Type::mut)) {
                if (context.try_consume(Token::Type::question)) {
                    mutability.emplace(
                        ast::Mutability {
                            .parameter_name = extract_lower_id(context, "a mutability parameter name"),
                            .type           = ast::Mutability::Type::parameterized
                        }
                    );
                }
                else {
                    mutability.emplace(
                        ast::Mutability {
                            .type = ast::Mutability::Type::mut
                        }
                    );
                }
            }
            else if (context.try_consume(Token::Type::immut)) {
                mutability.emplace(
                    ast::Mutability {
                        .type = ast::Mutability::Type::immut
                    }
                );
            }

            return bu::map(std::move(mutability), bu::make<ast::Template_argument>);
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

auto parser::extract_qualified(ast::Root_qualifier&& root, Parse_context& context)
    -> ast::Qualified_name
{
    std::vector<ast::Qualifier> qualifiers;
    lexer::Token* template_argument_anchor = nullptr;

    auto extract_qualifier = [&]() -> bool {
        switch (auto& token = context.extract(); token.type) {
        case Token::Type::lower_name:
            qualifiers.emplace_back(
                ast::Qualifier::Lower {
                    token.as_identifier()
                }
            );
            return true;
        case Token::Type::upper_name:
            template_argument_anchor = context.pointer;

            qualifiers.emplace_back(
                ast::Qualifier::Upper {
                    parse_template_arguments(context),
                    token.as_identifier()
                }
            );
            return true;
        default:
            context.retreat();
            return false;
        }
    };

    if (extract_qualifier()) {
        while (context.try_consume(Token::Type::double_colon)) {
            if (!extract_qualifier()) {
                throw context.expected("an identifier");
            }
        }

        auto back = std::move(qualifiers.back());
        qualifiers.pop_back();

        auto primary = [&]() -> ast::Primary_qualifier {
            if (auto* upper = back.upper()) {
                // Template arguments are handled separately
                upper->template_arguments.reset();
                context.pointer = template_argument_anchor;
                return { .identifier = upper->name, .uppercase = true };
            }
            else {
                return { .identifier = back.lower()->name, .uppercase = false };
            }
        }();

        return {
            .root_qualifier    = std::move(root),
            .middle_qualifiers = std::move(qualifiers),
            .primary_qualifier = std::move(primary)
        };
    }
    else {
        // root:: followed by no qualifiers
        throw context.expected("an identifier");
    }
}

auto parser::extract_mutability(Parse_context& context) -> ast::Mutability {
    ast::Mutability mutability;

    if (context.try_consume(Token::Type::mut)) {
        if (context.try_consume(Token::Type::question)) {
            mutability.type           = ast::Mutability::Type::parameterized;
            mutability.parameter_name = extract_lower_id(context, "a mutability parameter name");
        }
        else {
            mutability.type = ast::Mutability::Type::mut;
        }
    }
    else {
        mutability.type = ast::Mutability::Type::immut;
    }

    return mutability;
}

auto parser::token_description(Token::Type const type) -> std::string_view {
    switch (type) {
    case Token::Type::dot:           return "a '.'";
    case Token::Type::comma:         return "a ','";
    case Token::Type::colon:         return "a ':'";
    case Token::Type::semicolon:     return "a ';'";
    case Token::Type::double_colon:  return "a '::'";
    case Token::Type::ampersand:     return "a '&'";
    case Token::Type::asterisk:      return "a '*'";
    case Token::Type::question:      return "a '?'";
    case Token::Type::equals:        return "a '='";
    case Token::Type::pipe:          return "a '|'";
    case Token::Type::right_arrow:   return "a '->'";
    case Token::Type::paren_open:    return "a '('";
    case Token::Type::paren_close:   return "a ')'";
    case Token::Type::brace_open:    return "a '{'";
    case Token::Type::brace_close:   return "a '}'";
    case Token::Type::bracket_open:  return "a '['";
    case Token::Type::bracket_close: return "a ']'";

    case Token::Type::let:
    case Token::Type::mut:
    case Token::Type::immut:
    case Token::Type::if_:
    case Token::Type::else_:
    case Token::Type::for_:
    case Token::Type::in:
    case Token::Type::while_:
    case Token::Type::loop:
    case Token::Type::continue_:
    case Token::Type::break_:
    case Token::Type::match:
    case Token::Type::ret:
    case Token::Type::fn:
    case Token::Type::as:
    case Token::Type::data:
    case Token::Type::struct_:
    case Token::Type::class_:
    case Token::Type::inst:
    case Token::Type::alias:
    case Token::Type::import_:
    case Token::Type::export_:
    case Token::Type::module_:
    case Token::Type::size_of:
    case Token::Type::type_of:
    case Token::Type::meta:
    case Token::Type::where:
        return "a keyword";

    case Token::Type::underscore:    return "a wildcard pattern";
    case Token::Type::lower_name:    return "an uncapitalized identifier";
    case Token::Type::upper_name:    return "a capitalized identifier";
    case Token::Type::operator_name: return "an operator";
    case Token::Type::string:        return "a string literal";
    case Token::Type::integer:       return "an integer literal";
    case Token::Type::floating:      return "a floating-point literal";
    case Token::Type::character:     return "a character literal";
    case Token::Type::boolean:       return "a boolean literal";
    case Token::Type::end_of_input:  return "the end of input";
    default:
        bu::abort(std::format("Unimplemented for {}", type));
    }
}


namespace {

    using namespace parser;


    thread_local ast::Namespace                                 * current_namespace;
    thread_local decltype(ast::Module::instantiations)          * instantiations;
    thread_local decltype(ast::Module::instantiation_templates) * instantiation_templates;
    thread_local decltype(ast::Module::implementations)         * implementations;
    thread_local decltype(ast::Module::implementation_templates)* implementation_templates;

    template <class T>
    using Components = bu::Pair<std::optional<std::vector<ast::Template_parameter>>, T>;


    template <auto extract, auto concrete, auto non_concrete>
    auto extract_and_add_to(auto* const target, Parse_context& context) -> void {
        auto&& [template_parameters, definition] = extract(context);
        auto name = definition.name;

        auto const add_to_ordered = [&]<auto member>(bu::Value<member>) {
            if constexpr (std::same_as<decltype(target), ast::Namespace* const>) {
                target->definitions_in_order.push_back(std::addressof((target->*member).container().back().second));
            }
        };

        if (template_parameters) {
            (target->*non_concrete).add(
                std::move(name),
                ast::definition::Template_definition {
                    std::move(definition),
                    std::move(*template_parameters)
                }
            );

            add_to_ordered(bu::value<non_concrete>);
        }
        else {
            (target->*concrete).add(
                std::move(name),
                std::move(definition)
            );

            add_to_ordered(bu::value<concrete>);
        }
    }


    auto parse_class_reference(Parse_context&) -> std::optional<ast::Class_reference>;

    auto extract_classes(Parse_context& context) -> std::vector<ast::Class_reference> {
        constexpr auto extract_classes =
            extract_separated_zero_or_more<parse_class_reference, Token::Type::plus, "a class name">;

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

                if (context.try_consume(Token::Type::mut)) {
                    return ast::Template_parameter {
                        ast::Template_parameter::Mutability_parameter {
                            *name
                        }
                    };
                }
                else if (auto type = parse_type(context)) {
                    return ast::Template_parameter {
                        ast::Template_parameter::Value_parameter {
                            *name,
                            std::move(*type)
                        }
                    };
                }
                else {
                    throw context.expected("'mut' or a type");
                }
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

    auto extract_function_components(Parse_context& context)
        -> Components<ast::definition::Function>
    {
        auto const name                = extract_lower_id(context, "a function name");
        auto       template_parameters = parse_template_parameters(context);

        if (context.try_consume(Token::Type::paren_open)) {
            constexpr auto parse_parameters =
                extract_comma_separated_zero_or_more<parse_function_parameter, "a function parameter">;
            auto parameters = parse_parameters(context);
            context.consume_required(Token::Type::paren_close);

            std::optional<bu::Wrapper<ast::Type>> return_type;
            if (context.try_consume(Token::Type::colon)) {
                return_type.emplace(extract_type(context));
            }

            if (context.try_consume(Token::Type::where)) {
                bu::unimplemented(); // TODO: add support for where clauses
            }

            auto body = [&] {
                if (auto expression = parse_compound_expression(context)) {
                    return std::move(*expression);
                }
                else if (context.try_consume(Token::Type::equals)) {
                    return extract_expression(context);
                }
                else {
                    throw context.expected("the function body", "'=' or '{'");
                }
            }();

            return {
                std::move(template_parameters),
                ast::definition::Function {
                    std::move(body),
                    std::move(parameters),
                    name,
                    return_type
                }
            };
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

    auto extract_struct_components(Parse_context& context)
        -> Components<ast::definition::Struct>
    {
        constexpr auto parse_members =
            parse_comma_separated_one_or_more<parse_struct_member, "a struct member">;

        auto const name                = extract_upper_id(context, "a struct name");
        auto       template_parameters = parse_template_parameters(context);

        context.consume_required(Token::Type::equals);

        if (auto members = parse_members(context)) {
            return {
                std::move(template_parameters),
                ast::definition::Struct {
                    std::move(*members),
                    name
                }
            };
        }
        else {
            throw context.expected("one or more struct members");
        }
    }


    auto parse_data_constructor(Parse_context& context)
        -> std::optional<ast::definition::Data::Constructor>
    {
        if (auto name = parse_lower_id(context)) {
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

    auto extract_data_components(Parse_context& context)
        -> Components<ast::definition::Data>
    {
        constexpr auto parse_constructors =
            parse_separated_one_or_more<parse_data_constructor, Token::Type::pipe, "a data constructor">;

        auto* anchor = context.pointer;

        auto const name                = extract_upper_id(context, "a data name");
        auto       template_parameters = parse_template_parameters(context);

        context.consume_required(Token::Type::equals);
        if (auto constructors = parse_constructors(context)) {
            static constexpr auto max =
                std::numeric_limits<bu::U8>::max();

            if (constructors->size() > max) {
                // This allows the tag to always be a single byte
                throw context.error(
                    { anchor - 1, anchor + 1 },
                    std::format(
                        "A data-definition must not define more "
                        "than {} constructors, but {} defines {}",
                        max,
                        name,
                        constructors->size()
                    ),
                    "If this is truly necessary, consider categorizing "
                    "the constructors under several simpler types"
                );
            }

            return {
                std::move(template_parameters),
                ast::definition::Data {
                    std::move(*constructors),
                    name
                }
            };
        }
        else {
            throw context.expected("one or more data constructors");
        }
    }


    auto extract_alias_components(Parse_context& context)
        -> Components<ast::definition::Alias>
    {
        auto const name                = extract_upper_id(context, "an alias name");
        auto       template_parameters = parse_template_parameters(context);

        context.consume_required(Token::Type::equals);

        return {
            std::move(template_parameters),
            ast::definition::Alias {
                name,
                extract_type(context)
            }
        };
    }


    auto parse_class_reference(Parse_context& context)
        -> std::optional<ast::Class_reference>
    {
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

            if (name.primary_qualifier.is_upper()) {
                return name;
            }
            else {
                throw context.error(
                    { anchor, context.pointer },
                    "Expected a class name, but found a lowercase identifier"
                );
            }
        }();

        if (name) {
            return ast::Class_reference {
                .template_arguments = parse_template_arguments(context),
                .name               = std::move(*name)
            };
        }
        else {
            return std::nullopt;
        }
    }


    auto extract_impl_or_inst_definitions(auto* const target, Parse_context& context) -> void {
        context.consume_required(Token::Type::brace_open);

        for (;;) {
            switch (context.extract().type) {
            case Token::Type::fn:
                extract_and_add_to<
                    extract_function_components,
                    &ast::definition::Instantiation::function_definitions,
                    &ast::definition::Instantiation::function_template_definitions
                >(target, context);
                break;
            case Token::Type::alias:
                extract_and_add_to<
                    extract_alias_components,
                    &ast::definition::Instantiation::alias_definitions,
                    &ast::definition::Instantiation::alias_template_definitions
                >(target, context);
                break;
            case Token::Type::brace_close:
                return;
            default:
                context.retreat();
                throw context.expected(
                    "a function definition, an alias definition, or a closing '}'"
                );
            }
        }
    }


    auto extract_implementation(Parse_context& context) -> void {
        if (current_namespace != current_namespace->global) {
            context.retreat();
            throw context.error("Implementation blocks may only appear at global scope");
        }

        auto template_parameters = parse_template_parameters(context);

        ast::definition::Implementation implementation {
            .type = extract_type(context)
        };

        extract_impl_or_inst_definitions(&implementation, context);

        if (template_parameters) {
            implementation_templates->emplace_back(
                std::move(implementation),
                std::move(*template_parameters)
            );
        }
        else {
            implementations->push_back(std::move(implementation));
        }
    }


    auto extract_instantiation(Parse_context& context) -> void {
        if (current_namespace != current_namespace->global) {
            context.retreat();
            throw context.error("Instantiation blocks may only appear at global scope");
        }

        auto template_parameters = parse_template_parameters(context);

        if (auto typeclass = parse_class_reference(context)) {
            ast::definition::Instantiation instantiation {
                .typeclass = std::move(*typeclass),
                .instance  = extract_type(context)
            };

            extract_impl_or_inst_definitions(&instantiation, context);

            if (template_parameters) {
                instantiation_templates->emplace_back(
                    std::move(instantiation),
                    std::move(*template_parameters)
                );
            }
            else {
                instantiations->push_back(std::move(instantiation));
            }
        }
        else {
            throw context.expected("a class name");
        }
    }


    auto extract_function_signature(Parse_context& context) -> ast::Function_signature {
        auto name = extract_lower_id(context, "a function name");
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
        auto name = extract_upper_id(context, "an alias name");

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

    auto extract_class_components(Parse_context& context)
        -> Components<ast::definition::Typeclass>
    {
        auto name = extract_upper_id(context, "a class name");

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
                    throw context.expected("one or more function or type signatures");
                }
                if (is_braced) {
                    context.consume_required(Token::Type::brace_close);
                }

                return {
                    std::move(template_parameters),
                    ast::definition::Typeclass {
                        std::move(function_signatures),
                        std::move(type_signatures),
                        name
                    }
                };
            }
        }
    }


    auto parse_definition(Parse_context&) -> bool;

    auto extract_namespace(Parse_context& context) -> void {
        auto name = extract_lower_id(context, "a namespace name");

        context.consume_required(Token::Type::brace_open);

        auto* parent_namespace = current_namespace;

        current_namespace = parent_namespace->make_child(name);
        while (parse_definition(context));
        current_namespace = parent_namespace;

        context.consume_required(Token::Type::brace_close);
    }


    auto parse_definition(Parse_context& context) -> bool {
        switch (context.extract().type) {
        case Token::Type::fn:
            extract_and_add_to<
                extract_function_components,
                &ast::Namespace::function_definitions,
                &ast::Namespace::function_template_definitions
            >(current_namespace, context);
            break;
        case Token::Type::struct_:
            extract_and_add_to<
                extract_struct_components,
                &ast::Namespace::struct_definitions,
                &ast::Namespace::struct_template_definitions
            >(current_namespace, context);
            break;
        case Token::Type::data:
            extract_and_add_to<
                extract_data_components,
                &ast::Namespace::data_definitions,
                &ast::Namespace::data_template_definitions
            >(current_namespace, context);
            break;
        case Token::Type::alias:
            extract_and_add_to<
                extract_alias_components,
                &ast::Namespace::alias_definitions,
                &ast::Namespace::alias_template_definitions
            >(current_namespace, context);
            break;
        case Token::Type::class_:
            extract_and_add_to<
                extract_class_components,
                &ast::Namespace::class_definitions,
                &ast::Namespace::class_template_definitions
            >(current_namespace, context);
            break;
        case Token::Type::impl:
            extract_implementation(context);
            break;
        case Token::Type::inst:
            extract_instantiation(context);
            break;
        case Token::Type::namespace_:
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


    std::vector<ast::Import>         module_imports;
    std::optional<lexer::Identifier> module_name;

    if (context.try_consume(Token::Type::module_)) {
        module_name = extract_lower_id(context, "a module name");
    }

    while (context.try_consume(Token::Type::import_)) {
        static constexpr auto parse_path =
            parse_separated_one_or_more<parse_lower_id, Token::Type::dot, "a module qualifier">;

        if (auto path = parse_path(context)) {
            ast::Import import_statement;
            import_statement.path = std::move(*path);

            if (context.try_consume(Token::Type::as)) {
                import_statement.alias = extract_lower_id(context, "a module alias");
            }

            module_imports.push_back(std::move(import_statement));
        }
        else {
            throw context.expected("a module path");
        }
    }

    decltype(ast::Module::instantiations)           instantiations;
    decltype(ast::Module::instantiation_templates)  instantiation_templates;
    decltype(ast::Module::implementations)          implementations;
    decltype(ast::Module::implementation_templates) implementation_templates;
    ::instantiations           = &instantiations;
    ::instantiation_templates  = &instantiation_templates;
    ::implementations          = &implementations;
    ::implementation_templates = &implementation_templates;

    ::current_namespace = &global_namespace;

    while (parse_definition(context));

    if (!context.is_finished()) {
        throw context.expected(
            "a definition",
            "'fn', 'struct', 'data', 'alias', 'impl', 'inst', or 'class'"
        );
    }

    return ast::Module {
        .source                   = std::move(tokenized_source.source),
        .global_namespace         = std::move(global_namespace),
        .name                     = std::move(module_name),
        .imports                  = std::move(module_imports),
        .instantiations           = std::move(instantiations),
        .instantiation_templates  = std::move(instantiation_templates),
        .implementations          = std::move(implementations),
        .implementation_templates = std::move(implementation_templates),
    };
}