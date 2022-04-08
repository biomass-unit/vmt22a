#pragma once

#include "bu/utilities.hpp"
#include "bu/flatmap.hpp"
#include "bu/color.hpp"
#include "ast/ast.hpp"
#include "ast/ast_formatting.hpp"
#include "ir/ir.hpp"
#include "ir/ir_formatting.hpp"


namespace resolution {

    struct Binding {
        bu::Wrapper<ir::Type> type;
        vm::Local_size_type   frame_offset       = 0;
        ast::Expression*      moved_by           = nullptr;
        bool                  is_mutable         = false;
        bool                  has_been_mentioned = false;
    };

    struct Local_type_alias {
        bu::Wrapper<ir::Type> type;
        bool                  has_been_mentioned = false;
    };

    struct Scope {
        bu::Flatmap<lexer::Identifier, Binding>          bindings;
        bu::Flatmap<lexer::Identifier, Local_type_alias> local_type_aliases;
        std::vector<bu::Wrapper<ir::Type>>               destroy_in_reverse_order;
        bu::Bounded_u16                                  current_frame_offset;
        Scope*                                           parent = nullptr;

        auto make_child() noexcept -> Scope;

        auto find     (lexer::Identifier) noexcept -> Binding*;
        auto find_type(lexer::Identifier) noexcept -> Local_type_alias*;

        auto unused_variables() -> std::optional<std::vector<lexer::Identifier>>;
    };

    struct Namespace;


    namespace dtl {
        template <class>
        struct AST_to_IR_impl;

        template <> struct AST_to_IR_impl<ast::definition::Function > : std::type_identity<ir::definition::Function > {};
        template <> struct AST_to_IR_impl<ast::definition::Struct   > : std::type_identity<ir::definition::Struct   > {};
        template <> struct AST_to_IR_impl<ast::definition::Data     > : std::type_identity<ir::definition::Data     > {};
        template <> struct AST_to_IR_impl<ast::definition::Alias    > : std::type_identity<ir::definition::Alias    > {};
        template <> struct AST_to_IR_impl<ast::definition::Typeclass> : std::type_identity<ir::definition::Typeclass> {};
    }

    template <class T>
    using AST_to_IR = dtl::AST_to_IR_impl<T>::type;


    template <class T>
    struct Definition {
        struct Resolved_info {
            bu::Wrapper<AST_to_IR<T>> resolved;
            bu::Wrapper<ir::Type>     type_handle;

            // The type_handle, for type definitions, represents the only handle to the resolved
            // type, and for function definitions, represents a handle to the function type.
        };

        T*                                        syntactic_definition;
        bu::Wrapper<Namespace>                    home_namespace;
        bu::Wrapper<std::optional<Resolved_info>> resolved_info;

        [[nodiscard]]
        auto has_been_resolved() const noexcept -> bool {
            return resolved_info->has_value();
        }
    };

    template <class T>
    struct Definition<ast::definition::Template_definition<T>> {
        using Instantiation_info = Definition<T>::Resolved_info;

        T*                                                                      syntactic_definition;
        bu::Wrapper<Namespace>                                                  home_namespace;
        std::span<ast::Template_parameter>                                      template_parameters;
        bu::Wrapper<bu::Flatmap<ir::Template_argument_set, Instantiation_info>> instantiations;
    };

    using Function_definition           = Definition<ast::definition::Function          >;
    using Function_template_definition  = Definition<ast::definition::Function_template >;
    using Struct_definition             = Definition<ast::definition::Struct            >;
    using Struct_template_definition    = Definition<ast::definition::Struct_template   >;
    using Data_definition               = Definition<ast::definition::Data              >;
    using Data_template_definition      = Definition<ast::definition::Data_template     >;
    using Alias_definition              = Definition<ast::definition::Alias             >;
    using Alias_template_definition     = Definition<ast::definition::Alias_template    >;
    using Typeclass_definition          = Definition<ast::definition::Typeclass         >;
    using Typeclass_template_definition = Definition<ast::definition::Typeclass_template>;


    using Definition_variant = std::variant<
        Function_definition,
        Function_template_definition,
        Struct_definition,
        Struct_template_definition,
        Data_definition,
        Data_template_definition,
        Alias_definition,
        Alias_template_definition,
        Typeclass_definition,
        Typeclass_template_definition,

        bu::Wrapper<Namespace>
    >;

    static_assert(std::is_trivially_copyable_v<Definition_variant>);


    using Upper_variant = std::variant<
        Struct_definition,
        Struct_template_definition,
        Data_definition,
        Data_template_definition,
        Alias_definition,
        Alias_template_definition,
        Typeclass_definition,
        Typeclass_template_definition
    >;

    using Lower_variant = std::variant<
        Binding*,
        Function_definition,
        Function_template_definition
    >;


    struct Resolution_context;


    struct Namespace {
        template <class T>
        using Table = bu::Flatmap<lexer::Identifier, T>;

        std::vector<Definition_variant>       definitions_in_order;
        Table<Lower_variant>                  lower_table;
        Table<Upper_variant>                  upper_table;
        Table<bu::Wrapper<Namespace>>         children;
        std::optional<bu::Wrapper<Namespace>> parent;
        std::optional<lexer::Identifier>      name;

        auto find_type_here(lexer::Identifier,
                            bu::Source_view,
                            std::optional<std::span<ast::Template_argument>>,
                            Resolution_context&)
            -> std::optional<bu::Wrapper<ir::Type>>;

        auto format_name_as_member(lexer::Identifier) const -> std::string;
    };


    struct Resolution_context {
        Scope                                 scope;
        bu::Wrapper<Namespace>                current_namespace;
        bu::Wrapper<Namespace>                global_namespace;
        bu::Source                          * source               = nullptr;
        bu::Flatmap<lexer::Identifier, bool>* mutability_arguments = nullptr;
        bool                                  is_unevaluated       = false;

        auto make_child_context_with_new_scope() noexcept -> Resolution_context;


        auto find_upper(ast::Qualified_name&, bu::Source_view) -> Upper_variant;
        auto find_lower(ast::Qualified_name&, bu::Source_view) -> Lower_variant;

        auto find_type(ast::Qualified_name&,
                       bu::Source_view,
                       std::optional<std::span<ast::Template_argument>>)
            -> bu::Wrapper<ir::Type>;

        auto find_variable_or_function(ast::Qualified_name&,
                                       ast::Expression&,
                                       std::optional<std::span<ast::Template_argument>>)
            -> ir::Expression;


        auto get_associated_namespace(bu::Wrapper<ir::Type>, bu::Source_view)
            -> bu::Wrapper<Namespace>;

        auto resolve_mutability(ast::Mutability) -> bool;

        auto bind(ast::Pattern&, bu::Wrapper<ir::Type>) -> void;

        auto error(bu::Source_view  source_view,
                   std::string_view message,
                   std::optional<std::string_view> help = std::nullopt)
            -> std::runtime_error;

    };


    auto resolve_type      (ast::Type      &, Resolution_context&) -> bu::Wrapper<ir::Type>;
    auto resolve_expression(ast::Expression&, Resolution_context&) -> ir::Expression;

    auto resolve_definition(Definition_variant, Resolution_context&) -> void;

    auto resolve_template_arguments(Namespace&                         current_namespace,
                                    lexer::Identifier                  name,
                                    bu::Source_view                    source_view,
                                    std::span<ast::Template_parameter> parameters,
                                    std::span<ast::Template_argument>  arguments,
                                    Resolution_context&                context)
        -> ir::Template_argument_set;

}