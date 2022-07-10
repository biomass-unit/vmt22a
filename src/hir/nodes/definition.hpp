#ifndef VMT22A_HIR_NODES_DEFINITION
#define VMT22A_HIR_NODES_DEFINITION
#else
#error This isn't supposed to be included by anything other than hir/hir.hpp
#endif


namespace hir {

    namespace definition {

        struct Function {
            std::vector<Template_parameter>  explicit_template_parameters;
            std::vector<Template_parameter>  implicit_template_parameters;
            std::vector<Function_parameter>  parameters;
            std::optional<bu::Wrapper<Type>> return_type;
            DEFAULTED_EQUALITY(Function);
        };

    }

    struct Definition {
        using Variant = std::variant<
            definition::Function
        >;

        Variant         variant;
        bu::Source_view source_view;

        DEFAULTED_EQUALITY(Definition);
    };

}