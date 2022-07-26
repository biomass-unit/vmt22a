#ifndef VMT22A_HIR_NODES_DEFINITION
#define VMT22A_HIR_NODES_DEFINITION
#else
#error This isn't supposed to be included by anything other than hir/hir.hpp
#endif


namespace hir {

    using Function_signature = ast::Basic_function_signature <HIR_configuration>;
    using Type_signature     = ast::Basic_type_signature     <HIR_configuration>;


    namespace definition {

        struct Function {
            Template_parameters                      explicit_template_parameters;
            std::vector<Implicit_template_parameter> implicit_template_parameters;
            std::vector<Function_parameter>          parameters;
            std::optional<Type>                      return_type;
            Expression                               body;
            ast::Name                                name;
            DEFAULTED_EQUALITY(Function);
        };


        using Struct         = ast::definition::Basic_struct         <HIR_configuration>;
        using Enum           = ast::definition::Basic_enum           <HIR_configuration>;
        using Alias          = ast::definition::Basic_alias          <HIR_configuration>;
        using Typeclass      = ast::definition::Basic_typeclass      <HIR_configuration>;
        using Implementation = ast::definition::Basic_implementation <HIR_configuration>;
        using Instantiation  = ast::definition::Basic_instantiation  <HIR_configuration>;
        using Namespace      = ast::definition::Basic_namespace      <HIR_configuration>;

    }

    struct Definition {
        using Variant = std::variant<
            definition::Function,
            definition::Struct,
            definition::Enum,
            definition::Alias,
            definition::Typeclass,
            definition::Implementation,
            definition::Instantiation,
            definition::Namespace
        >;

        Variant         value;
        bu::Source_view source_view;

        DEFAULTED_EQUALITY(Definition);
    };

}