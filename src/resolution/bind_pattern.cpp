#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Pattern_bind_visitor {
        resolution::Resolution_context& context;
        ast::Pattern&                   this_pattern;
        bu::Wrapper<ir::Type>           this_type;

        inline static lexer::Identifier const wildcard_identifier { "_"sv };

        auto error(
            std::string_view                const message,
            std::optional<std::string_view> const help = std::nullopt
        )
            -> resolution::Resolution_context::Error
        {
            return context.error(
                this_pattern.source_view,
                message,
                help
            );
        }


        auto operator()(ast::pattern::Wildcard&, auto&) -> void {
            static ast::Pattern name {
                .value = ast::pattern::Name {
                    .identifier = wildcard_identifier,
                    .mutability {
                        .type        = ast::Mutability::Type::immut,
                        .source_view = this_pattern.source_view
                    },
                },
                .source_view = this_pattern.source_view
            };
            context.bind(name, this_type);
        }

        auto operator()(ast::pattern::Name& pattern, auto&) -> void {
            auto& bindings = context.scope.bindings.container();

            // emplace inserts the pair right before the given iterator; if
            // the name is already bound to something, this effectively shadows
            // it, and if the identifier is new, then ranges::find returns a
            // past-the-end iterator, meaning the pair is inserted at the end.

            auto const it = std::ranges::find(bindings, pattern.identifier, bu::first);

            if (it != bindings.end() && !it->second.has_been_mentioned) {
                auto const sections = std::to_array({
                    bu::Highlighted_text_section {
                        .source_view = it->second.source_view,
                        .source      = context.source,
                        .note        = "Unused variable declared here",
                        .note_color  = bu::text_warning_color
                    },
                    bu::Highlighted_text_section {
                        .source_view = this_pattern.source_view,
                        .source      = context.source,
                        .note        = "Shadowed here",
                        .note_color  = bu::text_error_color
                    }
                });

                auto const message = std::format(
                    "{} would shadow an unused local variable",
                    pattern.identifier
                );

                throw resolution::Resolution_context::Error {
                    bu::textual_error({
                        .sections = sections,
                        .source   = context.source,
                        .message  = message
                    })
                };
            }

            bindings.emplace(
                it,
                pattern.identifier,
                resolution::Binding {
                    .source_view        = this_pattern.source_view,
                    .type               = this_type,
                    .frame_offset       = context.scope.current_frame_offset.get(),
                    .is_mutable         = context.resolve_mutability(pattern.mutability),
                    .has_been_mentioned = pattern.identifier.view().starts_with('_')
                }
            );

            if (!context.is_unevaluated) {
                context.scope.current_frame_offset += this_type->size;

                if (!this_type->is_trivial) {
                    context.scope.destroy_in_reverse_order.push_back(this_type);
                }
            }
        }

        auto operator()(ast::pattern::Tuple& pattern, ir::type::Tuple& type) -> void {
            if (pattern.patterns.size() == type.types.size()) {
                for (bu::Usize i = 0; i != type.types.size(); ++i) {
                    context.bind(pattern.patterns[i], type.types[i]);
                }
            }
            else {
                throw error(
                    std::format(
                        "The tuple pattern contains {} patterns, but type {} contains {} types",
                        pattern.patterns.size(),
                        this_type,
                        type.types.size()
                    )
                );
            }
        }

        auto operator()(auto&, auto&) -> void {
            throw error(std::format("{} can not be bound to this pattern", this_type));
        }
    };

}


auto resolution::Resolution_context::bind(ast::Pattern& pattern, bu::Wrapper<ir::Type> type) -> void {
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