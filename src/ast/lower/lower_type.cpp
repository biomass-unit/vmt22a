#include "bu/utilities.hpp"
#include "lowering_internals.hpp"


namespace {

    struct Type_lowering_visitor {
        Lowering_context& context;
        ast::Type  const& this_type;

        template <class T>
        auto operator()(ast::type::Primitive<T> const& primitive) -> hir::Type::Variant {
            return primitive;
        }

        auto operator()(ast::type::Wildcard const&) -> hir::Type::Variant {
            return hir::type::Wildcard {};
        }

        auto operator()(ast::type::Typename const& name) -> hir::Type::Variant {
            return hir::type::Typename { context.lower(name.identifier) };
        }

        auto operator()(ast::type::Tuple const& tuple) -> hir::Type::Variant {
            return hir::type::Tuple {
                .types = bu::map(context.lower())(tuple.types)
            };
        }

        auto operator()(ast::type::Array const& array) -> hir::Type::Variant {
            return hir::type::Array {
                .element_type = context.lower(array.element_type),
                .length       = context.lower(array.length)
            };
        }

        auto operator()(ast::type::Slice const& slice) -> hir::Type::Variant {
            return hir::type::Slice {
                .element_type = context.lower(slice.element_type)
            };
        }

        auto operator()(ast::type::Function const& function) -> hir::Type::Variant {
            return hir::type::Function {
                .argument_types = bu::map(context.lower())(function.argument_types),
                .return_type    = context.lower(function.return_type)
            };
        }

        auto operator()(ast::type::Type_of const& type_of) -> hir::Type::Variant {
            return hir::type::Type_of {
                .expression = context.lower(type_of.expression)
            };
        }

        auto operator()(ast::type::Reference const& reference) -> hir::Type::Variant {
            return hir::type::Reference {
                .type       = context.lower(reference.type),
                .mutability = reference.mutability
            };
        }

        auto operator()(ast::type::Pointer const& pointer) -> hir::Type::Variant {
            return hir::type::Pointer {
                .type       = context.lower(pointer.type),
                .mutability = pointer.mutability
            };
        }

        auto operator()(ast::type::Instance_of const& instance_of) -> hir::Type::Variant {
            if (context.current_function_implicit_template_parameters) {
                // Within a function's parameter list or return type, inst types
                // are converted into references to implicit template parameters.

                auto const tag = context.fresh_name_tag();

                context.current_function_implicit_template_parameters->emplace_back(
                    bu::map(context.lower())(instance_of.classes),
                    hir::Implicit_template_parameter::Tag { tag }
                );

                return hir::type::Implicit_parameter_reference { .tag = tag };
            }
            else if (context.is_within_function()) {
                // Within a function body, inst types are simply used for constraint collection.

                return hir::type::Instance_of {
                    .classes = bu::map(context.lower())(instance_of.classes)
                };
            }
            else {
                context.error(this_type.source_view, { "'inst' types are only usable within functions" });
            }
        }

        auto operator()(ast::type::Template_application const& application) -> hir::Type::Variant {
            return hir::type::Template_application {
                .arguments = bu::map(context.lower())(application.arguments),
                .name      = context.lower(application.name)
            };
        }

        auto operator()(ast::type::For_all const& for_all) -> hir::Type::Variant {
            return hir::type::For_all {
                .parameters = bu::map(context.lower())(for_all.parameters),
                .body       = context.lower(for_all.body)
            };
        }
    };

}


auto Lowering_context::lower(ast::Type const& type) -> hir::Type {
    return {
        .value = std::visit(
            Type_lowering_visitor {
                .context   = *this,
                .this_type = type
            },
            type.value
        ),
        .source_view = type.source_view
    };
}