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


    using Upper_variant = std::variant<
        ast::definition::Struct             *,
        ast::definition::Struct_template    *,
        ast::definition::Data               *,
        ast::definition::Data_template      *,
        ast::definition::Alias              *,
        ast::definition::Alias_template     *,
        ast::definition::Typeclass          *,
        ast::definition::Typeclass_template *
    >;

    using Lower_variant = std::variant<
        Binding                            *,
        ast::definition::Function          *,
        ast::definition::Function_template *
    >;


    struct Namespace {
        template <class T>
        using Table = bu::Flatmap<lexer::Identifier, T>;

        Table<Lower_variant> lower_table;
        Table<Upper_variant> upper_table;

        Table<Namespace> children;
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