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
        hir::Name  name;
    };

    struct Struct {
        struct Member {
            hir::Name         name;
            bu::Wrapper<Type> type;
            bool              is_public;
        };
        Template_parameter_set template_parameters;
        std::vector<Member>    members;
        hir::Name              name;
    };

    struct Enum {
        struct Constructor {
            hir::Name                        name;
            std::optional<bu::Wrapper<Type>> type;
        };
        Template_parameter_set   template_parameters;
        std::vector<Constructor> constructors;
        hir::Name                name;
    };

    struct Alias {
        Template_parameter_set template_parameters;
        bu::Wrapper<Type>      aliased_type;
        hir::Name              name;
    };

    struct Typeclass {};

}