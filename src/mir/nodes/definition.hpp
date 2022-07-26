#ifndef VMT22A_MIR_NODES_DEFINITION
#define VMT22A_MIR_NODES_DEFINITION
#else
#error This isn't supposed to be included by anything other than mir/mir.hpp
#endif


namespace mir {

    struct Function {
        struct Signature {
            mir::Template_parameter_set          template_parameters;
            std::vector<mir::Function_parameter> parameters;
            bu::Wrapper<mir::Type>               return_type;
        };
        Signature  signature;
        Expression body;
        ast::Name  name;
    };

    struct Struct {
        struct Member {
            ast::Name         name;
            bu::Wrapper<Type> type;
            bool              is_public;
        };
        Template_parameter_set template_parameters;
        std::vector<Member>    members;
        ast::Name              name;
    };

    struct Enum {
        struct Constructor {
            ast::Name                        name;
            std::optional<bu::Wrapper<Type>> type;
            bu::Wrapper<Type>                function_type;
        };
        Template_parameter_set   template_parameters;
        std::vector<Constructor> constructors;
        ast::Name                name;
    };

    struct Alias {
        Template_parameter_set template_parameters;
        bu::Wrapper<Type>      aliased_type;
        ast::Name              name;
    };

    struct Typeclass {};

}