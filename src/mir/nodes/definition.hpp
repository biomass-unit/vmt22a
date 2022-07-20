#ifndef VMT22A_MIR_NODES_DEFINITION
#define VMT22A_MIR_NODES_DEFINITION
#else
#error This isn't supposed to be included by anything other than mir/mir.hpp
#endif


namespace mir {

    struct Function {
        Template_parameter_set          template_parameters;
        std::vector<Function_parameter> parameters;
        Type                            return_type;
        Expression                      body;
    };

    struct Struct {
        struct Member {
            hir::Name name;
            Type      type;
        };
        Template_parameter_set                 template_parameters;
        bu::Flatmap<lexer::Identifier, Member> members;
    };

    struct Enum {};

    struct Alias {};

    struct Typeclass {};


    struct Namespace {
        using Context = bu::Wrapper_context_for<Function, Struct, Enum, Alias, Typeclass, Namespace>;

        template <class T>
        using Table = bu::Flatmap<lexer::Identifier, bu::Wrapper<T>>;

        std::vector<
            std::variant<
                bu::Wrapper<Function>,
                bu::Wrapper<Struct>,
                bu::Wrapper<Enum>,
                bu::Wrapper<Alias>,
                bu::Wrapper<Typeclass>,
                bu::Wrapper<Namespace>
            >
        > definitions_in_order;

        Table<Function>  functions;
        Table<Struct>    structures;
        Table<Enum>      enumerations;
        Table<Alias>     aliases;
        Table<Typeclass> typeclasses;
        Table<Namespace> namespaces;

        Template_parameter_set   template_parameters;
        std::optional<hir::Name> name;
    };

}