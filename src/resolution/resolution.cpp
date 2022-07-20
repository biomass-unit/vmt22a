#include "bu/utilities.hpp"
#include "resolution.hpp"
#include "resolution_internals.hpp"
#include "lir/lir.hpp"


auto resolution::Context::error(
    bu::Source_view                    const source_view,
    bu::diagnostics::Message_arguments const arguments) -> void
{
    diagnostics.emit_simple_error(arguments.add_source_info(source, source_view));
}


namespace {

    template <class HIR_definition, auto resolution::Namespace::* member>
    auto visit_handler(resolution::Namespace& space) {
        return [&space](HIR_definition& hir_definition) -> void {
            lexer::Identifier identifier = hir_definition.name.identifier;
            bu::Wrapper definition = resolution::Definition_info { std::move(hir_definition) };
            (space.*member).add(std::move(identifier), bu::copy(definition));
            space.definitions_in_order.push_back(definition);
        };
    }

    auto register_namespace(
        resolution::Context                    & context,
        std::span<hir::Definition>         const definitions,
        bu::Wrapper<resolution::Namespace> const space) -> void
    {
        space->definitions_in_order.reserve(definitions.size());

        for (hir::Definition& definition : definitions) {
            std::visit(bu::Overload {
                visit_handler<hir::definition::Function,  &resolution::Namespace::functions>   (space),
                visit_handler<hir::definition::Struct,    &resolution::Namespace::structures>  (space),
                visit_handler<hir::definition::Enum,      &resolution::Namespace::enumerations>(space),
                visit_handler<hir::definition::Alias,     &resolution::Namespace::aliases>     (space),
                visit_handler<hir::definition::Typeclass, &resolution::Namespace::typeclasses> (space),

                [&](hir::definition::Implementation&) {
                    bu::todo();
                },
                [&](hir::definition::Instantiation&) {
                    bu::todo();
                },

                [&](hir::definition::Namespace& hir_child) {
                    if (hir_child.template_parameters.vector.has_value()) {
                        context.error(definition.source_view, { "Namespace templates are not supported yet" });
                    }

                    bu::Wrapper child = resolution::Namespace {
                        .name   = hir_child.name,
                        .parent = space
                    };
                    space->namespaces.add(bu::copy(hir_child.name.identifier), bu::copy(child));
                    space->definitions_in_order.push_back(child);
                    register_namespace(context, hir_child.definitions, child);
                }
            }, definition.value);
        }
    }

    auto register_top_level_definitions(hir::Module&& module) -> resolution::Context {
        resolution::Context context {
            std::move(module.node_context),
            resolution::Namespace::Context {},
            std::move(module.diagnostics),
            std::move(module.source),
        };
        register_namespace(context, module.definitions, context.global_namespace);
        return context;
    }

}


resolution::Context::Context(
    hir::Node_context       && node_context,
    Namespace::Context      && namespace_context,
    bu::diagnostics::Builder&& diagnostics,
    bu::Source              && source
) noexcept
    : hir_node_context { std::move(node_context) }
    , mir_node_context {
        bu::Wrapper_context<mir::Expression> { hir_node_context.arena_size<hir::Expression>() },
        bu::Wrapper_context<mir::Pattern>    { hir_node_context.arena_size<hir::Pattern>() },
        bu::Wrapper_context<mir::Type>       { hir_node_context.arena_size<hir::Type>() },
    }
    , namespace_context { std::move(namespace_context) }
    , diagnostics       { std::move(diagnostics) }
    , source            { std::move(source) } {}


auto resolution::resolve(hir::Module&& module) -> Module {
    [[maybe_unused]] auto context = register_top_level_definitions(std::move(module));
    bu::todo();
}