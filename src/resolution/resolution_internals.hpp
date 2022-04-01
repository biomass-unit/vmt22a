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

    struct Scope {
        bu::Flatmap<lexer::Identifier, Binding> bindings;
        std::vector<bu::Wrapper<ir::Type>>      destroy_in_reverse_order;
        bu::Bounded_u16                         current_frame_offset;
        Scope*                                  parent = nullptr;

        auto make_child() noexcept -> Scope;

        auto find(lexer::Identifier) noexcept -> Binding*;

        auto unused_variables() -> std::optional<std::vector<lexer::Identifier>>;
    };


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
        T*                                       definition;
        bu::Wrapper<std::optional<AST_to_IR<T>>> resolved;
    };

    template <class T>
    struct Definition<ast::definition::Template_definition<T>> {
        ast::definition::Template_definition<T>* definition;
        bu::Wrapper<std::vector<AST_to_IR<T>>>   instantiations;
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
        Typeclass_template_definition
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


    struct Namespace {
        template <class T>
        using Table = bu::Flatmap<lexer::Identifier, T>;

        std::vector<Definition_variant> definitions_in_order;

        Table<Lower_variant> lower_table;
        Table<Upper_variant> upper_table;
        Table<Namespace>     children;
    };


    struct Resolution_context {
        Scope                                 scope;
        Namespace                           * current_namespace     = nullptr;
        Namespace                           * global_namespace      = nullptr;
        bu::Source                          * source                = nullptr;
        bu::Flatmap<lexer::Identifier, bool>* mutability_parameters = nullptr;
        bool                                  is_unevaluated        = false;

        auto make_child_context_with_new_scope() noexcept -> Resolution_context {
            return {
                scope.make_child(),
                current_namespace,
                global_namespace,
                source,
                mutability_parameters,
                is_unevaluated
            };
        }

        auto find_upper(ast::Qualified_name& name) -> std::optional<Upper_variant> {
            assert(name.primary_qualifier.is_upper);
            return find_impl<&Namespace::upper_table>(name);
        }

        auto find_lower(ast::Qualified_name& name) -> std::optional<Lower_variant> {
            assert(!name.primary_qualifier.is_upper);
            if (name.is_unqualified()) {
                if (auto* binding = scope.find(name.primary_qualifier.name)) {
                    return binding;
                }
            }
            return find_impl<&Namespace::lower_table>(name);
        }

        auto resolve_mutability(ast::Mutability) -> bool;

    private:

        template <auto Namespace::* member>
        auto find_impl(ast::Qualified_name& name)
            -> std::optional<typename std::remove_reference_t<decltype(current_namespace->*member)>::Value>
        {
            if (auto* const pointer = (apply_qualifiers(name)->*member).find(name.primary_qualifier.name)) {
                return *pointer;
            }
            else {
                return std::nullopt;
            }
        }

        auto apply_qualifiers(ast::Qualified_name&) -> Namespace*;

    };


    auto resolve_type      (ast::Type      &, Resolution_context&) -> ir::Type;
    auto resolve_expression(ast::Expression&, Resolution_context&) -> ir::Expression;

    auto resolve_upper(Upper_variant, Resolution_context&) -> void;
    auto resolve_lower(Lower_variant, Resolution_context&) -> void;

}