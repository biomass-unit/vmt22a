#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Pattern_bind_visitor {
        resolution::Resolution_context& context;
        ast::Pattern&                   this_pattern;
        bu::Wrapper<ir::Type>           this_type;

        auto operator()(ast::pattern::Name& pattern, auto&) -> void {
            auto& bindings = context.scope.bindings.container();

            // emplace inserts the pair right before the given iterator; if
            // the name is already bound to something, this effectively shadows
            // it, and if the identifier is new, then ranges::find returns a
            // past-the-end iterator, meaning the pair is inserted at the end.

            bindings.emplace(
                std::ranges::find(bindings, pattern.identifier, bu::first),
                pattern.identifier,
                resolution::Binding {
                    .type         = this_type,
                    .frame_offset = context.scope.current_frame_offset.get(),
                    .is_mutable   = context.resolve_mutability(pattern.mutability)
                }
            );

            if (!context.is_unevaluated) {
                context.scope.current_frame_offset.safe_add(this_type->size);
                context.scope.destroy_in_reverse_order.push_back(this_type);
            }
        }

        auto operator()(auto&, auto&) -> void {
            bu::unimplemented();
        }
    };

}


auto resolution::Resolution_context::bind(ast::Pattern& pattern, bu::Wrapper<ir::Type> const type) -> void {
    std::visit(
        Pattern_bind_visitor {
            .context      = *this,
            .this_pattern = pattern,
            .this_type    = type
        },
        pattern.value,
        type->value
    );
}