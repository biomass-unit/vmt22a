#include "bu/utilities.hpp"
#include "lowering_internals.hpp"


namespace {

    struct Pattern_lowering_visitor {
        Lowering_context  & context;
        ast::Pattern const& this_pattern;

        auto operator()(auto const&) -> hir::Pattern {
            bu::todo();
        }
    };

}


auto Lowering_context::lower(ast::Pattern const& pattern) -> hir::Pattern {
    return std::visit(
        Pattern_lowering_visitor {
            .context = *this,
            .this_pattern = pattern
        },
        pattern.value
    );
}