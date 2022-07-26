#ifndef VMT22A_AST_NODES_DEFINITION
#define VMT22A_AST_NODES_DEFINITION
#else
#error This isn't supposed to be included by anything other than ast/ast.hpp
#endif


namespace ast {

    template <tree_configuration Configuration>
    struct Basic_function_signature {
        Basic_template_parameters<Configuration>  template_parameters;
        std::vector<typename Configuration::Type> argument_types;
        Configuration::Type                       return_type;
        Name                                      name;
        DEFAULTED_EQUALITY(Basic_function_signature);
    };

    template <tree_configuration Configuration>
    struct Basic_type_signature {
        Basic_template_parameters<Configuration>          template_parameters;
        std::vector<Basic_class_reference<Configuration>> classes;
        Name                                              name;
        DEFAULTED_EQUALITY(Basic_type_signature);
    };

    using Function_signature = Basic_function_signature <AST_configuration>;
    using Type_signature     = Basic_type_signature     <AST_configuration>;


    namespace definition {

        struct Function {
            // ast::Function and hir::Function are different enough that Basic_function would only complicate
            // things, so ast::definition::Function and hir::definition::Function are entirely separate.

            Expression                      body;
            std::vector<Function_parameter> parameters;
            Name                            name;
            std::optional<Type>             return_type;
            Template_parameters             template_parameters;
            DEFAULTED_EQUALITY(Function);
        };


        template <tree_configuration Configuration>
        struct Basic_struct_member {
            Name                name;
            Configuration::Type type;
            bool                is_public = false;
            bu::Source_view     source_view;
            DEFAULTED_EQUALITY(Basic_struct_member);
        };

        template <tree_configuration Configuration>
        struct Basic_struct {
            using Member = Basic_struct_member<Configuration>;
            std::vector<Member>                      members;
            Name                                     name;
            Basic_template_parameters<Configuration> template_parameters;
            DEFAULTED_EQUALITY(Basic_struct);
        };


        template <tree_configuration Configuration>
        struct Basic_enum_constructor {
            Name                                        name;
            std::optional<typename Configuration::Type> type;
            bu::Source_view                             source_view;
            DEFAULTED_EQUALITY(Basic_enum_constructor);
        };

        template <tree_configuration Configuration>
        struct Basic_enum {
            using Constructor = Basic_enum_constructor<Configuration>;
            std::vector<Constructor>                 constructors;
            Name                                     name;
            Basic_template_parameters<Configuration> template_parameters;
            DEFAULTED_EQUALITY(Basic_enum);
        };


        template <tree_configuration Configuration>
        struct Basic_alias {
            Name                                     name;
            Configuration::Type                      type;
            Basic_template_parameters<Configuration> template_parameters;
            DEFAULTED_EQUALITY(Basic_alias);
        };


        template <tree_configuration Configuration>
        struct Basic_typeclass {
            std::vector<Basic_function_signature<Configuration>> function_signatures;
            std::vector<Basic_type_signature<Configuration>>     type_signatures;
            Name                                                 name;
            Basic_template_parameters<Configuration>             template_parameters;
            DEFAULTED_EQUALITY(Basic_typeclass);
        };


        template <tree_configuration Configuration>
        struct Basic_implementation {
            Configuration::Type                             type;
            std::vector<typename Configuration::Definition> definitions;
            Basic_template_parameters<Configuration>        template_parameters;
            DEFAULTED_EQUALITY(Basic_implementation);
        };


        template <tree_configuration Configuration>
        struct Basic_instantiation {
            Basic_class_reference<Configuration>            typeclass;
            Configuration::Type                             instance;
            std::vector<typename Configuration::Definition> definitions;
            Basic_template_parameters<Configuration>        template_parameters;
            DEFAULTED_EQUALITY(Basic_instantiation);
        };


        template <tree_configuration Configuration>
        struct Basic_namespace {
            std::vector<typename Configuration::Definition> definitions;
            Name                                            name;
            Basic_template_parameters<Configuration>        template_parameters;
            DEFAULTED_EQUALITY(Basic_namespace);
        };


        using Struct         = Basic_struct         <AST_configuration>;
        using Enum           = Basic_enum           <AST_configuration>;
        using Alias          = Basic_alias          <AST_configuration>;
        using Typeclass      = Basic_typeclass      <AST_configuration>;
        using Implementation = Basic_implementation <AST_configuration>;
        using Instantiation  = Basic_instantiation  <AST_configuration>;
        using Namespace      = Basic_namespace      <AST_configuration>;

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