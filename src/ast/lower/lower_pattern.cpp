#include "bu/utilities.hpp"
#include "lowering_internals.hpp"


namespace {

    struct Pattern_lowering_visitor {
        Lowering_context  & context;
        ast::Pattern const& this_pattern;

        template <class T>
        auto operator()(ast::pattern::Literal<T> const& literal) -> hir::Pattern {
            return {
                .value = literal,
                .source_view = this_pattern.source_view
            };
        }

        auto operator()(ast::pattern::Wildcard const&) -> hir::Pattern {
            return {
                .value = hir::pattern::Wildcard {},
                .source_view = this_pattern.source_view
            };
        }

        auto operator()(ast::pattern::Name const& name) -> hir::Pattern {
            return {
                .value = hir::pattern::Name {
                    .identifier = name.identifier,
                    .mutability = name.mutability
                },
                .source_view = this_pattern.source_view
            };
        }

        auto operator()(ast::pattern::Tuple const& tuple) -> hir::Pattern {
            return {
                .value = hir::pattern::Tuple {
                    .patterns = bu::map(context.lower())(tuple.patterns)
                },
                .source_view = this_pattern.source_view
            };
        }

        auto operator()(ast::pattern::Slice const& slice) -> hir::Pattern {
            return {
                .value = hir::pattern::Slice {
                    .patterns = bu::map(context.lower())(slice.patterns)
                },
                .source_view = this_pattern.source_view
            };
        }

        auto operator()(ast::pattern::Constructor const& ctor) -> hir::Pattern {
            return {
                .value = hir::pattern::Constructor {
                    .name    = context.lower(ctor.name),
                    .pattern = ctor.pattern.transform(bu::compose(bu::wrap, context.lower()))
                },
                .source_view = this_pattern.source_view
            };
        }

        auto operator()(ast::pattern::Constructor_shorthand const& ctor) -> hir::Pattern {
            return {
                .value = hir::pattern::Constructor_shorthand {
                    .name    = context.lower(ctor.name),
                    .pattern = ctor.pattern.transform(bu::compose(bu::wrap, context.lower()))
                },
                .source_view = this_pattern.source_view
            };
        }

        auto operator()(ast::pattern::As const& as) -> hir::Pattern {
            return {
                .value = hir::pattern::As {
                    .name    = as.name,
                    .pattern = context.lower(as.pattern)
                },
                .source_view = this_pattern.source_view
            };
        }

        auto operator()(ast::pattern::Guarded const& guarded) -> hir::Pattern {
            return {
                .value = hir::pattern::Guarded {
                    .pattern = context.lower(guarded.pattern),
                    .guard   = context.lower(guarded.guard)
                },
                .source_view = this_pattern.source_view
            };
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