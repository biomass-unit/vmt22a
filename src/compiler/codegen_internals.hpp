#pragma once

#include "bu/utilities.hpp"
#include "bu/flatmap.hpp"
#include "bu/textual_error.hpp"
#include "ast/ast.hpp"
#include "vm/virtual_machine.hpp"


namespace compiler {

    struct Scope {
        struct Binding {
            bu::Wrapper<ast::Type> type;
            vm::Local_offset_type  frame_offset;
        };

        bu::Flatmap<lexer::Identifier, Binding> bindings;
        vm::Virtual_machine*                    machine;
        Scope*                                  parent;

        auto find(lexer::Identifier) noexcept -> Binding*;

        auto make_child() & -> Scope;
    };

    struct Codegen_context {
        vm::Virtual_machine machine;
        ast::Module         module;
        ast::Namespace*     space;
        Scope               scope;

        Codegen_context(vm::Virtual_machine&& machine, ast::Module&& module) noexcept
            : machine { std::move(machine) }
            , module  { std::move(module) }
            , space   { &this->module.global_namespace }
            , scope   { .machine = &this->machine, .parent = nullptr } {}

        auto error(std::string_view                message,
                   auto const&                     erroneous_node,
                   std::optional<std::string_view> help = std::nullopt)
            -> std::runtime_error
        {
            return std::runtime_error {
                bu::textual_error({
                    .erroneous_view = erroneous_node.source_view,
                    .file_view      = module.source.string(),
                    .file_name      = module.source.name(),
                    .message        = message,
                    .help_note      = help
                })
            };
        }
    };


    auto size_of(ast::Type&      , Codegen_context&) -> vm::Local_size_type;
    auto type_of(ast::Expression&, Codegen_context&) -> ast::Type&;

}