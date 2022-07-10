#ifndef VMT22A_AST_NODES_DEFINITION
#define VMT22A_AST_NODES_DEFINITION
#else
#error This isn't supposed to be included by anything other than ast/ast.hpp
#endif


namespace ast {

    struct Template_parameter {
        struct Type_parameter {
            std::vector<Class_reference> classes;
            DEFAULTED_EQUALITY(Type_parameter);
        };
        struct Value_parameter {
            std::optional<bu::Wrapper<Type>> type;
            DEFAULTED_EQUALITY(Value_parameter);
        };
        struct Mutability_parameter {
            DEFAULTED_EQUALITY(Mutability_parameter);
        };

        using Variant = std::variant<
            Type_parameter,
            Value_parameter,
            Mutability_parameter
        >;

        Variant         value;
        Name            name;
        bu::Source_view source_view;
        DEFAULTED_EQUALITY(Template_parameter);
    };


    struct Template_parameters {
        std::optional<std::vector<Template_parameter>> vector;
        Template_parameters(decltype(vector)&& vector) noexcept
            : vector { std::move(vector) } {}
    };


    struct Function_signature {
        std::optional<std::vector<Template_parameter>> template_parameters;
        type::Function                                 type;
        Name                                           name;
        DEFAULTED_EQUALITY(Function_signature);
    };

    struct Type_signature {
        std::optional<std::vector<Template_parameter>> template_parameters;
        std::vector<Class_reference>                   classes;
        Name                                           name;
        DEFAULTED_EQUALITY(Type_signature);
    };


    namespace definition {

        struct Function {
            Expression                       body;
            std::vector<Function_parameter>  parameters;
            Name                             name;
            std::optional<bu::Wrapper<Type>> return_type;
            Template_parameters              template_parameters;
            DEFAULTED_EQUALITY(Function);
        };

        struct Struct {
            struct Member {
                lexer::Identifier name;
                bu::Wrapper<Type> type;
                bool              is_public = false;

                bu::Source_view source_view;
                DEFAULTED_EQUALITY(Member);
            };
            std::vector<Member> members;
            Name                name;
            Template_parameters template_parameters;
            DEFAULTED_EQUALITY(Struct);
        };

        struct Enum {
            struct Constructor {
                lexer::Identifier                name;
                std::optional<bu::Wrapper<Type>> type;

                bu::Source_view source_view;
                DEFAULTED_EQUALITY(Constructor);
            };
            std::vector<Constructor> constructors;
            Name                     name;
            Template_parameters      template_parameters;
            DEFAULTED_EQUALITY(Enum);
        };

        struct Alias {
            Name                name;
            bu::Wrapper<Type>   type;
            Template_parameters template_parameters;
            DEFAULTED_EQUALITY(Alias);
        };

        struct Typeclass {
            std::vector<Function_signature> function_signatures;
            std::vector<Type_signature>     type_signatures;
            Name                            name;
            Template_parameters             template_parameters;
            DEFAULTED_EQUALITY(Typeclass);
        };

        struct Implementation {
            bu::Wrapper<Type>       type;
            std::vector<Definition> definitions;
            Template_parameters     template_parameters;
            DEFAULTED_EQUALITY(Implementation);
        };

        struct Instantiation {
            Class_reference         typeclass;
            bu::Wrapper<Type>       instance;
            std::vector<Definition> definitions;
            Template_parameters     template_parameters;
            DEFAULTED_EQUALITY(Instantiation);
        };

        struct Namespace {
            std::vector<Definition> definitions;
            Name                    name;
            Template_parameters     template_parameters;
            DEFAULTED_EQUALITY(Namespace);
        };

    }


    struct Definition {
        using Variant = std::variant<
            definition::Function,
            definition::Enum,
            definition::Struct,
            definition::Alias,
            definition::Typeclass,
            definition::Instantiation,
            definition::Implementation,
            definition::Namespace
        >;

        Variant         value;
        bu::Source_view source_view;
    };

}