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


    template <class HIR_representation, class MIR_representation>
    struct Definition_info : std::variant<HIR_representation, MIR_representation> {
        Definition_state state = Definition_state::unresolved;
    };

    template <>
    struct Definition_info<hir::definition::Function, mir::Function> :
        std::variant<
            hir::definition::Function,   // Fully unresolved
            Partially_resolved_function, // Signature resolved, body unresolved
            bu::Wrapper<mir::Function>   // Fully resolved
        >
    {
        Definition_state state = Definition_state::unresolved;
    };

    using Function_info  = Definition_info<hir::definition::Function,  mir::Function>;
    using Struct_info    = Definition_info<hir::definition::Struct,    mir::Struct>;
    using Enum_info      = Definition_info<hir::definition::Enum,      mir::Enum>;
    using Alias_info     = Definition_info<hir::definition::Alias,     mir::Alias>;
    using Typeclass_info = Definition_info<hir::definition::Typeclass, mir::Typeclass>;
    
    Definition_info(hir::definition::Function&&)  -> Function_info;
    Definition_info(hir::definition::Struct&&)    -> Struct_info;
    Definition_info(hir::definition::Enum&&)      -> Enum_info;
    Definition_info(hir::definition::Alias&&)     -> Alias_info;
    Definition_info(hir::definition::Typeclass&&) -> Typeclass_info;


    struct Namespace {
        using Context = bu::Wrapper_context_for<
            Function_info,
            Struct_info,
            Enum_info,
            Alias_info,
            Typeclass_info,
            Namespace
        >;

        template <class T>
        using Table = bu::Flatmap<lexer::Identifier, bu::Wrapper<T>>;

        using Definition_variant = std::variant<
            bu::Wrapper<Function_info>,
            bu::Wrapper<Struct_info>,
            bu::Wrapper<Enum_info>,
            bu::Wrapper<Alias_info>,
            bu::Wrapper<Typeclass_info>,
            bu::Wrapper<Namespace>
        >;

        std::vector<Definition_variant> definitions_in_order;

        Table<Function_info>  functions;
        Table<Struct_info>    structures;
        Table<Enum_info>      enumerations;
        Table<Alias_info>     aliases;
        Table<Typeclass_info> typeclasses;
        Table<Namespace>      namespaces;

        mir::Template_parameter_set           template_parameters;
        std::optional<hir::Name>              name;
        std::optional<bu::Wrapper<Namespace>> parent;
    };


    class Context {
        bu::Safe_usize     current_type_variable_tag;
        hir::Node_context  hir_node_context;
        mir::Node_context  mir_node_context;
        Namespace::Context namespace_context;
    public:
        bu::diagnostics::Builder diagnostics;
        bu::Source               source;
        bu::Wrapper<Namespace>   global_namespace;

        Context(hir::Node_context&&, Namespace::Context&&, bu::diagnostics::Builder&&, bu::Source&&) noexcept;

        Context(Context const&) = delete;
        Context(Context&&) = default;


        auto fresh_type_variable(mir::type::Variable::Kind = mir::type::Variable::Kind::general) -> mir::Type;

        auto error(bu::Source_view, bu::diagnostics::Message_arguments) -> void;


        auto resolve(hir::Expression const&) -> mir::Expression;

        auto resolve() noexcept {
            return [this](auto const& node) {
                return resolve(node);
            };
        }
    };

}