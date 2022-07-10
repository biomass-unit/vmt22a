#include "bu/utilities.hpp"
#include "lowering_internals.hpp"


namespace {

    struct Type_lowering_visitor {
        Lowering_context& node_context;
        ast::Type const & this_type;

        auto operator()(auto const&) -> hir::Type {
            bu::todo();
        }
    };

}


auto Lowering_context::lower(ast::Type const& type) -> hir::Type {
    return std::visit(
        Type_lowering_visitor {
            .node_context = *this,
            .this_type = type
        },
        type.value
    );
}