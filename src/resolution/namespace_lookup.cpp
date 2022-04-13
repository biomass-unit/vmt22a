#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    auto get_namespace_name(resolution::Namespace& space)
        noexcept -> std::string_view
    {
        return space.name
            .transform(&lexer::Identifier::view)
            .value_or("The global namespace"sv);
    }


    auto find_root(ast::Root_qualifier&            root,
                   resolution::Resolution_context& context)
        -> bu::Pair<bool, bu::Wrapper<resolution::Namespace>>
    {
        bool is_absolute = true;

        bu::wrapper auto space = std::visit(bu::Overload {
            [&](std::monostate) {
                is_absolute = false;
                return context.current_namespace;
            },
            [&](ast::Root_qualifier::Global) {
                return context.global_namespace;
            },
            [&](ast::Type& type) -> bu::Wrapper<resolution::Namespace> {
                return context.get_associated_namespace(context.resolve_type(type), type.source_view);
            }
        }, root.value);

        return { is_absolute, space };
    }


    auto apply_qualifiers(bu::Wrapper<resolution::Namespace> space,
                          std::span<ast::Qualifier> const    qualifiers,
                          resolution::Resolution_context&    context)
        -> resolution::Namespace*
    {
        for (ast::Qualifier& qualifier : qualifiers) {
            if (qualifier.name.is_upper) {
                auto type = space->find_type_here(
                    qualifier.name.identifier,
                    qualifier.name.source_view,
                    qualifier.template_arguments.transform(
                        bu::make<std::span<ast::Template_argument>>
                    ),
                    context
                );
                if (type) {
                    space = context.get_associated_namespace(*type, qualifier.name.source_view);
                }
                else {
                    throw context.error(
                        qualifier.name.source_view,
                        std::format(
                            "{} does not contain a {} {}",
                            get_namespace_name(space),
                            qualifier.template_arguments ? "template" : "type",
                            qualifier.name
                        )
                    );
                }
            }
            else {
                if (qualifier.template_arguments) {
                    bu::unimplemented();
                }

                bu::wrapper auto* const child = space->children.find(qualifier.name.identifier);

                if (!child) {
                    throw context.error(
                        qualifier.name.source_view,
                        std::format(
                            "{} does not contain a namespace {}",
                            get_namespace_name(space),
                            qualifier.name
                        )
                    );
                }
                else {
                    space = *child;
                }
            }
        }

        return &*space;
    }


    auto apply_relative_qualifier(bu::Wrapper<resolution::Namespace> root,
                                  ast::Qualifier&                    first,
                                  resolution::Resolution_context&    context)
        -> bu::Wrapper<resolution::Namespace>
    {
        for (;;) {
            if (first.name.is_upper) {
                std::optional<bu::Wrapper<ir::Type>> type = root->find_type_here(
                    first.name.identifier,
                    first.source_view,
                    first.template_arguments.transform(
                        bu::make<std::span<ast::Template_argument>>
                    ),
                    context
                );

                if (type) {
                    root = context.get_associated_namespace(*type, first.source_view);
                    break;
                }
            }
            else {
                if (first.template_arguments) {
                    bu::unimplemented();
                }
                if (bu::Wrapper<resolution::Namespace>* const child = root->children.find(first.name.identifier)) {
                    root = *child;
                    break;
                }
            }

            if (root->parent) {
                root = *root->parent;
            }
            else {
                throw context.error(
                    first.source_view,
                    std::format("{} is undefined", first.name)
                );
            }
        }

        return root;
    }


    template <bool is_upper, auto resolution::Namespace::* lookup_table>
    auto find_impl(ast::Qualified_name&            full_name,
                   bu::Source_view const           source_view,
                   resolution::Resolution_context& context)
        -> std::conditional_t<is_upper, resolution::Upper_variant, resolution::Lower_variant>
    {
        auto const primary = full_name.primary_name;
        assert(primary.is_upper == is_upper);

        auto [is_absolute, root] = find_root(full_name.root_qualifier, context);

        std::span<ast::Qualifier> qualifiers = full_name.middle_qualifiers;

        if (!is_absolute) {
            /*

                If the path is not absolute, we must search for a match for the first qualifier. Without
                this step, the following code wouldn't work, as `example::g` would not be able to find `::f`
                
                fn f() ()
                
                namespace example {
                    fn g() = f()
                }

            */

            if (qualifiers.empty()) {
                for (;;) {
                    if (auto* const pointer = ((&*root)->*lookup_table).find(primary.identifier)) {
                        return *pointer;
                    }
                    else if (root->parent) {
                        root = *root->parent;
                    }
                    else {
                        throw context.error(
                            source_view,
                            std::format("{} is undefined", primary.identifier)
                        );
                    }
                }
            }
            else {
                root = apply_relative_qualifier(root, qualifiers.front(), context);
                qualifiers = qualifiers.subspan<1>(); // Pop the first qualifier
            }
        }

        resolution::Namespace* const space =
            apply_qualifiers(root, qualifiers, context);

        if (auto* const pointer = (space->*lookup_table).find(primary.identifier)) {
            return *pointer;
        }
        else {
            throw context.error(
                primary.source_view,
                std::format(
                    "{} does not contain a definition for {}",
                    get_namespace_name(*space),
                    primary.identifier
                )
            );
        }
    }

}


auto resolution::Resolution_context::find_upper(ast::Qualified_name& full_name, bu::Source_view const source_view) -> Upper_variant {
    return find_impl<true, &Namespace::upper_table>(full_name, source_view, *this);
}

auto resolution::Resolution_context::find_lower(ast::Qualified_name& full_name, bu::Source_view const source_view) -> Lower_variant {
    return find_impl<false, &Namespace::lower_table>(full_name, source_view, *this);
}


auto resolution::Resolution_context::get_associated_namespace_if(bu::Wrapper<ir::Type> type)
    noexcept -> std::optional<bu::Wrapper<Namespace>>
{
    if (auto* const uds = std::get_if<ir::type::User_defined_struct>(&type->value)) {
        return uds->structure->associated_namespace;
    }
    else if (auto* const udd = std::get_if<ir::type::User_defined_data>(&type->value)) {
        return udd->data->associated_namespace;
    }
    else {
        return std::nullopt;
    }
}

auto resolution::Resolution_context::get_associated_namespace(
    bu::Wrapper<ir::Type> type,
    bu::Source_view const source_view
)
    -> bu::Wrapper<Namespace>
{
    if (auto space = get_associated_namespace_if(type)) {
        return *space;
    }
    else {
        throw error(
            source_view,
            std::format(
                "{} is not a user-defined type, so it "
                "does not have an associated namespace",
                type
            )
        );
    }
}