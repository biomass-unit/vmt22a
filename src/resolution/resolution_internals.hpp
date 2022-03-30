#pragma once

#include "bu/utilities.hpp"
#include "bu/flatmap.hpp"
#include "ast/ast.hpp"
#include "ast/ast_formatting.hpp"
#include "ir/ir.hpp"


namespace resolution {

    struct Binding {
        bu::Wrapper<ir::Type> type;
        bu::U16               frame_offset;
        ast::Expression*      moved_by           = nullptr;
        bool                  is_mutable         = false;
        bool                  has_been_mentioned = false;
    };

    struct Scope {
        bu::Flatmap<lexer::Identifier, Binding> bindings;
        std::vector<bu::Wrapper<ir::Type>>      destroy_in_reverse_order;
        Scope*                                  parent               = nullptr;
        bu::U16                                 current_frame_offset = 0;

        auto make_child() noexcept -> Scope;

        auto find(lexer::Identifier) noexcept -> Binding*;
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
        Scope scope;

        Namespace* current_namespace;
        Namespace* global_namespace;

        bool is_unevaluated = false;

        auto make_child_context_with_new_scope() noexcept -> Resolution_context {
            return {
                scope.make_child(),
                current_namespace,
                global_namespace,
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

        auto apply_qualifiers(ast::Qualified_name& name) -> Namespace* {
            auto* root = std::visit(bu::Overload {
                [&](std::monostate) {
                    return current_namespace;
                },
                [&](ast::Root_qualifier::Global) {
                    return global_namespace;
                },
                [](ast::Type&) -> Namespace* {
                    bu::unimplemented();
                }
            }, name.root_qualifier->value);

            for (auto& qualifier : name.middle_qualifiers) {
                if (qualifier.is_upper || qualifier.template_arguments) {
                    bu::unimplemented();
                }

                if (auto* const child = root->children.find(qualifier.name)) {
                    root = child;
                }
                else {
                    bu::unimplemented();
                }
            }

            return root;
        }

    };


    auto resolve_type      (ast::Type      &, Resolution_context&) -> ir::Type;
    auto resolve_expression(ast::Expression&, Resolution_context&) -> ir::Expression;

    auto resolve_upper(Upper_variant, Resolution_context&) -> void;
    auto resolve_lower(Lower_variant, Resolution_context&) -> void;

}