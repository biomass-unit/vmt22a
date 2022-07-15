#ifndef VMT22A_HIR_NODES_DEFINITION
#define VMT22A_HIR_NODES_DEFINITION
#else
#error This isn't supposed to be included by anything other than hir/hir.hpp
#endif


namespace hir {

    namespace definition {

        struct Function {
            Template_parameters                      explicit_template_parameters;
            std::vector<Implicit_template_parameter> implicit_template_parameters;
            std::vector<Function_parameter>          parameters;
            std::optional<bu::Wrapper<Type>>         return_type;
            bu::Wrapper<Expression>                  body;
            Name                                     name;
            DEFAULTED_EQUALITY(Function);
        };

        struct Struct {
            struct Member {
                lexer::Identifier name;
                bu::Wrapper<Type> type;
                bool              is_public;

                bu::Source_view source_view;
                DEFAULTED_EQUALITY(Member);
            };
            Template_parameters template_parameters;
            std::vector<Member> members;
            Name                name;
        };

        struct Enum {
            struct Constructor {
                lexer::Identifier   name;
                std::optional<Type> type;

                bu::Source_view source_view;
                DEFAULTED_EQUALITY(Constructor);
            };
            std::vector<Constructor> constructors;
            Name                     name;
            Template_parameters      template_parameters;
            DEFAULTED_EQUALITY(Enum);
        };

    }

    struct Definition {
        using Variant = std::variant<
            definition::Function,
            definition::Struct,
            definition::Enum
        >;

        Variant         value;
        bu::Source_view source_view;

        DEFAULTED_EQUALITY(Definition);
    };

}