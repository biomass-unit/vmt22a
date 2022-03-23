#pragma once

#include "bu/utilities.hpp"
#include "bu/flatmap.hpp"
#include "ast/ast.hpp"
#include "ir.hpp"


namespace compiler {

    struct Namespace {
        template <class T>
        using Table = bu::Flatmap<lexer::Identifier, T*>;

        Table<ast::definition::Function>           function_definitions;
        Table<ast::definition::Function_template>  function_template_definitions;
        Table<ast::definition::Struct>             struct_definitions;
        Table<ast::definition::Struct_template>    struct_template_definitions;
        Table<ast::definition::Data>               data_definitions;
        Table<ast::definition::Data_template>      data_template_definitions;
        Table<ast::definition::Alias>              alias_definitions;
        Table<ast::definition::Alias_template>     alias_template_definitions;
        Table<ast::definition::Typeclass>          class_definitions;
        Table<ast::definition::Typeclass_template> class_template_definitions;

        bu::Flatmap<lexer::Identifier, Namespace> children;
    };


    struct Resolution_context {
        Namespace* current_namespace;
        Namespace* global_namespace;

        template <auto Namespace::*... members>
        auto find_one_of(ast::Qualified_name& name) {
            if (name.is_unqualified()) {
                bu::unimplemented(); // try local scope first
            }

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

            return find_impl<
                std::variant<
                    std::monostate,
                    typename std::remove_reference_t<
                        decltype(current_namespace->*members)
                    >::Value...
                >,
                members...
            >(root, name.primary_qualifier.identifier);
        }

    private:

        template <class Variant, auto Namespace::* head, auto Namespace::*... tail>
        auto find_impl(Namespace* const space, lexer::Identifier const name) -> Variant {
            if (auto** const pointer = (space->*head).find(name)) {
                return *pointer;
            }
            else {
                if constexpr (sizeof...(tail) != 0) {
                    return find_impl<Variant, tail...>(space, name);
                }
                else {
                    return std::monostate {};
                }
            }
        }
    };


    auto resolve_type      (ast::Type&      , Resolution_context&) -> ir::Type;
    auto resolve_definition(ast::Definition&, Resolution_context&) -> void;

}