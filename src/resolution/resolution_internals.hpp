#pragma once

#include "bu/utilities.hpp"
#include "hir/hir.hpp"
#include "mir/mir.hpp"


namespace resolution {

    enum class Definition_state {
        unresolved,
        resolved,
        currently_on_resolution_stack,
    };
    

    class Scope {
    public:
        struct Variable_binding {
            bu::Wrapper<mir::Type>         type;
            ast::Mutability                mutability;
            bool                           has_been_mentioned;
            std::optional<bu::Source_view> source_view;
        };

        struct Type_binding {
            bu::Wrapper<mir::Type>         type;
            bool                           has_been_mentioned;
            std::optional<bu::Source_view> source_view;
        };
    private:
        bu::Flatmap<lexer::Identifier, Variable_binding> variables;
        bu::Flatmap<lexer::Identifier, Type_binding>     types;
        class Context*                                   context;
        Scope*                                           parent = nullptr;
    public:
        Scope(Context&) noexcept;

        Scope(Scope const&) = delete;
        Scope(Scope&&) noexcept;

        auto operator=(Scope const&) -> Scope& = delete;
        auto operator=(Scope&&) noexcept -> Scope&;

        auto bind_variable(lexer::Identifier, Variable_binding&&) -> void;
        auto bind_type    (lexer::Identifier, Type_binding    &&) -> void;

        auto find_variable(lexer::Identifier) noexcept -> Variable_binding*;
        auto find_type    (lexer::Identifier) noexcept -> Type_binding*;

        auto make_child() noexcept -> Scope;

        auto warn_about_unused_bindings() -> void;
    };


    struct Partially_resolved_function {
        mir::Function::Signature resolved_signature;
        Scope                    signature_scope;
        hir::Expression          unresolved_body;
        hir::Name                name;
    };


    struct Namespace;

    template <class HIR_representation, class MIR_representation>
    struct Definition_info {
        using Variant = std::variant<HIR_representation, MIR_representation>;

        Variant                value;
        bu::Wrapper<Namespace> home_namespace;
        Definition_state       state = Definition_state::unresolved;
    };

    template <>
    struct Definition_info<hir::definition::Function, mir::Function> {
        using Variant = std::variant<
            hir::definition::Function,   // Fully unresolved
            Partially_resolved_function, // Signature resolved, body unresolved
            mir::Function                // Fully resolved
        >;

        Variant                value;
        bu::Wrapper<Namespace> home_namespace;
        Definition_state       state = Definition_state::unresolved;
    };

    template <>
    struct Definition_info<hir::definition::Struct, mir::Struct> {
        using Variant = std::variant<hir::definition::Struct, mir::Struct>;

        Variant                value;
        bu::Wrapper<Namespace> home_namespace;
        bu::Wrapper<Namespace> associated_namespace;
        Definition_state       state;
    };

    template <>
    struct Definition_info<hir::definition::Enum, mir::Enum> {
        using Variant = std::variant<hir::definition::Enum, mir::Enum>;

        Variant                value;
        bu::Wrapper<Namespace> home_namespace;
        bu::Wrapper<Namespace> associated_namespace;
        Definition_state       state;
    };

    Definition_info(hir::definition::Function &&, bu::Wrapper<Namespace>) -> Function_info;
    Definition_info(hir::definition::Struct   &&, bu::Wrapper<Namespace>) -> Struct_info;
    Definition_info(hir::definition::Enum     &&, bu::Wrapper<Namespace>) -> Enum_info;
    Definition_info(hir::definition::Alias    &&, bu::Wrapper<Namespace>) -> Alias_info;
    Definition_info(hir::definition::Typeclass&&, bu::Wrapper<Namespace>) -> Typeclass_info;


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

        std::vector<Definition_variant>       definitions_in_order;

        Table<Function_info>                  functions;
        Table<Struct_info>                    structures;
        Table<Enum_info>                      enumerations;
        Table<Alias_info>                     aliases;
        Table<Typeclass_info>                 typeclasses;
        Table<Namespace>                      namespaces;

        mir::Template_parameter_set           template_parameters;
        std::optional<hir::Name>              name;
        std::optional<bu::Wrapper<Namespace>> parent;
    };


    namespace constraint {
        struct Equality {
            bu::Wrapper<mir::Type> left, right;
        };
        struct Instance {
            bu::Wrapper<mir::Type>            type;
            std::vector<mir::Class_reference> classes;
        };
    }

    struct Constraint_set {
        std::vector<constraint::Equality> equality_constraints;
        std::vector<constraint::Instance> instance_constraints;
    };


    class Context {
        hir::Node_context  hir_node_context;
        mir::Node_context  mir_node_context;
        Namespace::Context namespace_context;
    public:
        bu::diagnostics::Builder diagnostics;
        bu::Source               source;
        bu::Wrapper<Namespace>   global_namespace;

        bu::Wrapper<mir::Type> size_type;


        Context(
            hir::Node_context&&,
            mir::Node_context&&,
            Namespace::Context&&,
            bu::diagnostics::Builder&&,
            bu::Source&&
        ) noexcept;

        Context(Context const&) = delete;
        Context(Context&&) = default;


        [[noreturn]]
        auto error(bu::Source_view, bu::diagnostics::Message_arguments) -> void;

        auto unify(Constraint_set&) -> void;

        auto resolve_type      (hir::Type      &, Scope&, Namespace&) -> bu::Wrapper<mir::Type>;
        auto resolve_expression(hir::Expression&, Scope&, Namespace&) -> bu::Pair<Constraint_set, mir::Expression>;

        auto resolve_function_signature(Function_info&) -> mir::Function::Signature&;
        auto resolve_function          (Function_info&) -> mir::Function&;

        auto resolve_structure(Struct_info&) -> mir::Struct&;
        auto resolve_enumeration(Enum_info&) -> mir::Enum&;


        auto namespace_associated_with(mir::Type&) -> std::optional<bu::Wrapper<Namespace>>;


        auto find_typeclass(Scope&, Namespace&, hir::Qualified_name&)
            -> std::optional<bu::Wrapper<Typeclass_info>>;

        auto find_type(Scope&, Namespace&, hir::Qualified_name&)
            -> std::optional<std::variant<bu::Wrapper<Struct_info>, bu::Wrapper<Enum_info>, bu::Wrapper<Alias_info>>>;

        auto find_function(Scope&, Namespace&, hir::Qualified_name&)
            -> std::optional<bu::Wrapper<Function_info>>;
    };

}


DECLARE_FORMATTER_FOR(resolution::Constraint_set);