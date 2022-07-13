#pragma once

#include "bu/utilities.hpp"
#include "bu/bounded_integer.hpp"
#include "bu/diagnostics.hpp"
#include "ast/ast.hpp"
#include "hir/hir.hpp"


class Lowering_context {
    inline static bu::Bounded_usize current_name_tag = 0;
public:
    hir::Node_context       & node_context;
    bu::diagnostics::Builder& diagnostics;
    bu::Source         const& source;

    std::vector<hir::Template_parameter>* current_function_implicit_template_parameters = nullptr;

    auto lower(ast::Expression const&) -> hir::Expression;
    auto lower(ast::Type       const&) -> hir::Type;
    auto lower(ast::Pattern    const&) -> hir::Pattern;
    auto lower(ast::Definition const&) -> hir::Definition;

    auto lower() noexcept {
        return [this](auto const& node) {
            return lower(node);
        };
    }

    static auto fresh_lower_name() -> lexer::Identifier { return fresh_name('x'); }
    static auto fresh_upper_name() -> lexer::Identifier { return fresh_name('X'); }
private:
    static auto fresh_name(char const first) -> lexer::Identifier {
        // maybe use a dedicated string pool for compiler-inserted names?
        return lexer::Identifier {
            "{}#{}"_format(first, current_name_tag++),
            bu::Pooled_string_strategy::guaranteed_new_string
        };
    }
};