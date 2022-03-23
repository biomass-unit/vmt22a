#pragma once

#include "bu/utilities.hpp"
#include "bu/flatmap.hpp"
#include "ast/ast.hpp"
#include "ir.hpp"


namespace compiler {

    struct Namespace {
        template <class T>
        using Table = bu::Flatmap<lexer::Identifier, T*>;

        Table<ast::definition::Struct> struct_definitions;
        Table<ast::definition::Data>   data_definitions;
        // add other tables

        bu::Flatmap<lexer::Identifier, Namespace> children;
    };


    struct Resolution_context {
        Namespace* current_namespace;
        Namespace* global_namespace;

        template <auto Namespace::*... members>
        auto find_one_of(ast::Qualified_name& name)
            -> std::variant<decltype(&(current_namespace->*members))...>
        {
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

            std::ignore = root;

            bu::unimplemented();
        }
    };


    auto resolve_type(ast::Type&, Resolution_context&) -> ir::Type;

}