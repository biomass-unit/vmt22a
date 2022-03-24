#pragma once

#include "bu/utilities.hpp"
#include "bu/flatmap.hpp"
#include "ast/ast.hpp"
#include "ir.hpp"


namespace compiler {

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
        Namespace* current_namespace;
        Namespace* global_namespace;

        auto find_upper(ast::Qualified_name& name) -> std::optional<Upper_variant> {
            return find_impl<&Namespace::upper_table>(name);
        }

        auto find_lower(ast::Qualified_name& name) -> std::optional<Lower_variant> {
            return find_impl<&Namespace::lower_table>(name);
        }

    private:

        template <auto Namespace::* member>
        auto find_impl(ast::Qualified_name& name)
            -> std::optional<typename std::remove_reference_t<decltype(current_namespace->*member)>::Value>
        {
            if (auto* const pointer = (apply_qualifiers(name)->*member).find(name.primary_qualifier.identifier)) {
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
                if (auto* const lower = qualifier.lower()) {
                    if (auto* const child = root->children.find(lower->name)) {
                        root = child;
                    }
                    else {
                        bu::unimplemented();
                    }
                }
                else {
                    bu::unimplemented();
                }
            }

            return root;
        }

    };


    auto resolve_type (ast::Type&, Resolution_context&) -> ir::Type;

    auto resolve_upper(Upper_variant, Resolution_context&) -> void;
    auto resolve_lower(Lower_variant, Resolution_context&) -> void;

}