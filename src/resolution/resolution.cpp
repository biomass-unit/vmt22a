#include "bu/utilities.hpp"
#include "resolution.hpp"
#include "resolution_internals.hpp"
#include "lir/lir.hpp"


namespace {

    template <class HIR_definition, auto resolution::Namespace::* member>
    auto visit_handler(bu::Wrapper<resolution::Namespace> const space) {
        return [space](HIR_definition& hir_definition) -> void {
            lexer::Identifier identifier = hir_definition.name.identifier;
            bu::Wrapper definition = resolution::Definition_info { std::move(hir_definition), space };
            ((*space).*member).add(std::move(identifier), bu::copy(definition));
            space->definitions_in_order.push_back(definition);
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


    // Creates a resolution context by collecting top level name information
    auto register_top_level_definitions(hir::Module&& module) -> resolution::Context {
        resolution::Context context {
            std::move(module.hir_node_context),
            std::move(module.mir_node_context),
            resolution::Namespace::Context {},
            std::move(module.diagnostics),
            std::move(module.source),
        };
        register_namespace(context, module.definitions, context.global_namespace);
        return context;
    }


    // Resolves all definitions in order, but only visits function bodies if their return types have been omitted
    auto resolve_signatures(resolution::Context& context, bu::Wrapper<resolution::Namespace> const space) -> void {
        for (resolution::Namespace::Definition_variant& definition : space->definitions_in_order) {
            std::visit(bu::Overload {
                [&](resolution::Function_info& info) {
                    auto& sig = context.resolve_function_signature(info);
                    bu::print("return type: {}\n", sig.return_type);
                },
                [&](resolution::Struct_info&)    { bu::todo(); },
                [&](resolution::Enum_info&)      { bu::todo(); },
                [&](resolution::Alias_info&)     { bu::todo(); },
                [&](resolution::Typeclass_info&) { bu::todo(); },
                [&](bu::Wrapper<resolution::Namespace>& child) {
                    resolve_signatures(context, child);
                }
            }, definition);
        }
    }

}


auto resolution::resolve(hir::Module&& module) -> Module {
    Context context = register_top_level_definitions(std::move(module));
    resolve_signatures(context, context.global_namespace);
    bu::todo();
}