#include "bu/utilities.hpp"
#include "lowering_internals.hpp"


namespace {

    struct Pattern_lowering_visitor {
        Lowering_context& context;

        template <class T>
        auto operator()(ast::pattern::Literal<T> const& literal) -> hir::Pattern::Variant {
            return literal;
        }

        auto operator()(ast::pattern::Wildcard const&) -> hir::Pattern::Variant {
            return hir::pattern::Wildcard {};
        }

        auto operator()(ast::pattern::Name const& name) -> hir::Pattern::Variant {
            return hir::pattern::Name {
                .identifier = name.identifier,
                .mutability = name.mutability
            };
        }

        auto operator()(ast::pattern::Tuple const& tuple) -> hir::Pattern::Variant {
            return hir::pattern::Tuple {
                .patterns = bu::map(context.lower())(tuple.patterns)
            };
        }

        auto operator()(ast::pattern::Slice const& slice) -> hir::Pattern::Variant {
            return hir::pattern::Slice {
                .patterns = bu::map(context.lower())(slice.patterns)
            };
        }

        auto operator()(ast::pattern::Constructor const& ctor) -> hir::Pattern::Variant {
            return hir::pattern::Constructor {
                .name    = context.lower(ctor.name),
                .pattern = ctor.pattern.transform(bu::compose(bu::wrap, context.lower()))
            };
        }

        auto operator()(ast::pattern::Constructor_shorthand const& ctor) -> hir::Pattern::Variant {
            return hir::pattern::Constructor_shorthand {
                .name    = context.lower(ctor.name),
                .pattern = ctor.pattern.transform(bu::compose(bu::wrap, context.lower()))
            };
        }

        auto operator()(ast::pattern::As const& as) -> hir::Pattern::Variant {
            return hir::pattern::As {
                .name    = as.name,
                .pattern = context.lower(as.pattern)
            };
        }

        auto operator()(ast::pattern::Guarded const& guarded) -> hir::Pattern::Variant {
            return hir::pattern::Guarded {
                .pattern = context.lower(guarded.pattern),
                .guard   = context.lower(guarded.guard)
            };
        }
    };

}


auto Lowering_context::lower(ast::Pattern const& pattern) -> hir::Pattern {
    return {
        .value = std::visit(Pattern_lowering_visitor { .context = *this }, pattern.value),
        .source_view = pattern.source_view
    };
}