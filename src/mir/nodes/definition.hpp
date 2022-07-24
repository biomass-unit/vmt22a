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
    };

    struct Struct {
        struct Member {
            hir::Name         name;
            bu::Wrapper<Type> type;
            bool              is_public;
        };
        Template_parameter_set                 template_parameters;
        bu::Flatmap<lexer::Identifier, Member> members;
    };

    struct Enum {};

    struct Alias {};

    struct Typeclass {};

}