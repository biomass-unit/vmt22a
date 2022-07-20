#pragma once

#include "bu/utilities.hpp"
#include "bu/safe_integer.hpp"
#include "bu/diagnostics.hpp"
#include "ast/ast.hpp"
#include "hir/hir.hpp"
#include "mir/mir.hpp"


class Lowering_context {
    bu::Safe_usize current_name_tag;
    bu::Safe_usize current_type_variable_tag;
    bu::Usize      current_definition_kind = std::variant_size_v<ast::Definition::Variant>;
public:
    hir::Node_context             & node_context;
    bu::Wrapper_context<mir::Type>& type_context;
    bu::diagnostics::Builder      & diagnostics;
    bu::Source               const& source;

    bu::Wrapper<mir::Type>       unit_type      = bu::hole(); //mir::type::Tuple {};
    bu::Wrapper<mir::Type>       floating_type  = bu::hole(); //mir::type::Floating {};
    bu::Wrapper<mir::Type>       character_type = bu::hole(); //mir::type::Character {};
    bu::Wrapper<mir::Type>       boolean_type   = bu::hole(); //mir::type::Boolean {};
    bu::Wrapper<mir::Type>       string_type    = bu::hole(); //mir::type::String {};
    bu::Wrapper<hir::Expression> unit_value     { hir::expression::Tuple {}, unit_type };
    bu::Wrapper<hir::Pattern>    true_pattern   = hir::pattern::Literal<bool> { true };
    bu::Wrapper<hir::Pattern>    false_pattern  = hir::pattern::Literal<bool> { false };

    std::vector<hir::Implicit_template_parameter>* current_function_implicit_template_parameters = nullptr;


    Lowering_context(hir::Node_context&, bu::Wrapper_context<mir::Type>&, bu::diagnostics::Builder&, bu::Source const&) noexcept;


    auto is_within_function() const noexcept -> bool;

    auto fresh_name_tag() -> bu::Usize;

    auto fresh_type_variable(mir::type::Variable::Kind = mir::type::Variable::Kind::general) -> mir::Type;

    auto lower(ast::Expression const&) -> hir::Expression;
    auto lower(ast::Type       const&) -> hir::Type;
    auto lower(ast::Pattern    const&) -> hir::Pattern;
    auto lower(ast::Definition const&) -> hir::Definition;

    auto lower(ast::Function_argument   const&) -> hir::Function_argument;
    auto lower(ast::Function_parameter  const&) -> hir::Function_parameter;
    auto lower(ast::Template_argument   const&) -> hir::Template_argument;
    auto lower(ast::Template_parameter  const&) -> hir::Template_parameter;
    auto lower(ast::Template_parameters const&) -> hir::Template_parameters;
    auto lower(ast::Qualifier           const&) -> hir::Qualifier;
    auto lower(ast::Qualified_name      const&) -> hir::Qualified_name;
    auto lower(ast::Class_reference     const&) -> hir::Class_reference;
    auto lower(ast::Function_signature  const&) -> hir::Function_signature;
    auto lower(ast::Type_signature      const&) -> hir::Type_signature;

    // For convenience
    static auto lower(ast::Name const&) -> hir::Name;

    auto lower() noexcept {
        return [this](auto const& node) {
            return lower(node);
        };
    }

    [[noreturn]]
    auto error(bu::Source_view, bu::diagnostics::Message_arguments) const -> void;
};