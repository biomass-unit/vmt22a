#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


resolution::Scope::Scope(Context& context) noexcept
    : context { context } {}


resolution::Scope::~Scope() {
    auto const warn_about_unused_bindings = [this](auto const& bindings, std::string_view const formal_name) {
        for (auto& [name, binding] : bindings.container()) {
            if (!binding.has_been_mentioned
                && binding.source_view.has_value() // Ignore bindings that have been inserted by the compiler
                && name.view().front() != '_')     // Ignore bindings that start with an underscore
            {
                context.diagnostics.emit_simple_warning({
                    .erroneous_view      = *binding.source_view,
                    .source              = context.source,
                    .message             = "Unused local {}",
                    .message_arguments   = std::make_format_args(formal_name),
                    .help_note           = "If this is intentional, prefix the {} with an underscore: _{}",
                    .help_note_arguments = std::make_format_args(formal_name, name)
                });
            }
        }
    };

    warn_about_unused_bindings(variables, "variable");
    warn_about_unused_bindings(types, "type alias");
}


namespace {

    auto add_binding(resolution::Context& context, auto& bindings, lexer::Identifier const identifier, auto&& binding, std::string_view const description) -> void {
        auto const it = std::ranges::find(bindings, identifier, bu::first);

        if (it == bindings.end()) {
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


auto resolution::Scope::bind_variable(lexer::Identifier const identifier, Variable_binding&& binding) -> void {
    add_binding(context, variables.container(), identifier, std::move(binding), "variable");
}

auto resolution::Scope::bind_type(lexer::Identifier const identifier, Type_binding&& binding) -> void {
    add_binding(context, types.container(), identifier, std::move(binding), "type alias");
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
    Scope child { context };
    child.parent = this;
    return child;
}