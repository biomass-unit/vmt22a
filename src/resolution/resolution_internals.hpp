#pragma once

#include "bu/utilities.hpp"
#include "bu/safe_integer.hpp"
#include "hir/hir.hpp"
#include "mir/mir.hpp"


namespace resolution {

    enum class Definition_state {
        unresolved,
        resolved,
        currently_on_resolution_stack,
    };


    struct Partially_resolved_function {
        mir::Template_parameter_set          template_parameters;
        std::vector<mir::Function_parameter> parameters;
        mir::Type                            return_type;
        hir::Expression                      unresolved_body;
        ast::Name                            name;
    };

    struct Function_info :
        std::variant<
            hir::definition::Function,   // Fully unresolved
            Partially_resolved_function, // Signature resolved, body unresolved
            mir::Function                // Fully resolved
        >
    {
        Definition_state state = Definition_state::unresolved;
    };


    template <class HIR_representation, class MIR_representation>
    struct Definition_info : std::variant<HIR_representation, MIR_representation> {
        Definition_state state = Definition_state::unresolved;
    };

    using Struct_info    = Definition_info<hir::definition::Struct,    mir::Struct>;
    using Enum_info      = Definition_info<hir::definition::Enum,      mir::Enum>;
    using Alias_info     = Definition_info<hir::definition::Alias,     mir::Alias>;
    using Typeclass_info = Definition_info<hir::definition::Typeclass, mir::Typeclass>;
    

    class Context {
        bu::Safe_usize current_type_variable_tag;
    public:
        mir::Node_context       & node_context;
        mir::Namespace::Context & namespace_context;
        bu::diagnostics::Builder& diagnostics;

        Context(mir::Node_context&, mir::Namespace::Context&, bu::diagnostics::Builder&) noexcept;


        auto fresh_type_variable(mir::type::Variable::Kind = mir::type::Variable::Kind::general) -> mir::Type;


        auto resolve(hir::Expression const&) -> mir::Expression;

        auto resolve() noexcept {
            return [this](auto const& node) {
                return resolve(node);
            };
        }
    };

}