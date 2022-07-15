#ifndef VMT22A_AST_NODES_DEFINITION
#define VMT22A_AST_NODES_DEFINITION
#else
#error This isn't supposed to be included by anything other than ast/ast.hpp
#endif


namespace ast {

    struct Function_signature {
        Template_parameters template_parameters;
        type::Function      type;
        Name                name;
        DEFAULTED_EQUALITY(Function_signature);
    };

    struct Type_signature {
        Template_parameters          template_parameters;
        std::vector<Class_reference> classes;
        Name                         name;
        DEFAULTED_EQUALITY(Type_signature);
    };


    namespace definition {

        struct Function {
            Expression                      body;
            std::vector<Function_parameter> parameters;
            Name                            name;
            std::optional<Type>             return_type;
            Template_parameters             template_parameters;
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
            definition::Struct,
            definition::Enum,
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