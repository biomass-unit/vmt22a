#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    auto warn_about_unused_bindings(
        resolution::Context  & context,
        auto                 & bindings,
        std::string_view const description) -> void
    {
        for (auto& [name, binding] : bindings) {
            if (!binding.has_been_mentioned && binding.source_view.has_value()) {
                context.diagnostics.emit_simple_warning({
                    .erroneous_view      = *binding.source_view,
                    .source              = context.source,
                    .message             = "Unused local {}",
                    .message_arguments   = std::make_format_args(description),
                    .help_note           = "If this is intentional, prefix the {} with an underscore: _{}",
                    .help_note_arguments = std::make_format_args(description, name)
                });
            }
        }
    }

    auto add_binding(
        resolution::Context   & context,
        auto                  & bindings,
        lexer::Identifier const identifier,
        auto                 && binding,
        std::string_view  const description) -> void
    {
        // If the name starts with an underscore, then we pretend that the
        // binding has already been mentioned in order to prevent possible warnings.
        binding.has_been_mentioned = identifier.view().front() == '_';

        if (auto const it = std::ranges::find(bindings, identifier, bu::first); it == bindings.end()) {
            bindings.emplace_back(identifier, std::forward<decltype(binding)>(binding));
        }
        else {
            if (!it->second.has_been_mentioned && context.diagnostics.warning_level() != bu::diagnostics::Level::suppress) {
                context.diagnostics.emit_warning({
                    .sections = bu::vector_from<bu::diagnostics::Text_section>({
                        {
                            .source_view = bu::get(it->second.source_view),
                            .source      = context.source,
                            .note        = "First declared here"
                        },
                        {
                            .source_view = bu::get(binding.source_view),
                            .source      = context.source,
                            .note        = "Later shadowed here"
                        }
                    }),
                    .message             = "Local {} shadows an unused local {}",
                    .message_arguments   = std::make_format_args(description, description),
                    .help_note           = "If this is intentional, prefix the first {} with an underscore: _{}",
                    .help_note_arguments = std::make_format_args(description, it->first)
                });
                it->second.has_been_mentioned = true; // Prevent a second warning about the same variable
            }
            bindings.emplace(it, identifier, std::move(binding));
        }
    }

}


resolution::Scope::Scope(Context& context) noexcept
    : context { &context } {}


auto resolution::Scope::bind_variable(lexer::Identifier const identifier, Variable_binding&& binding) -> void {
    add_binding(*context, variables.container(), identifier, std::move(binding), "variable");
}

auto resolution::Scope::bind_type(lexer::Identifier const identifier, Type_binding&& binding) -> void {
    add_binding(*context, types.container(), identifier, std::move(binding), "type alias");
}


auto resolution::Scope::find_variable(lexer::Identifier const identifier) noexcept -> Variable_binding* {
    if (Variable_binding* const binding = variables.find(identifier)) {
        return binding;
    }
    else {
        return parent ? parent->find_variable(identifier) : nullptr;
    }
}

auto resolution::Scope::find_type(lexer::Identifier const identifier) noexcept -> Type_binding* {
    if (Type_binding* const binding = types.find(identifier)) {
        return binding;
    }
    else {
        return parent ? parent->find_type(identifier) : nullptr;
    }
}


auto resolution::Scope::make_child() noexcept -> Scope {
    Scope child { *context };
    child.parent = this;
    return child;
}


auto resolution::Scope::warn_about_unused_bindings() -> void {
    ::warn_about_unused_bindings(*context, variables.container(), "variable");
    ::warn_about_unused_bindings(*context, types.container(), "type alias");
}