#include "bu/utilities.hpp"
#include "lowering_internals.hpp"


namespace {

    struct Type_lowering_visitor {
        Lowering_context& context;
        ast::Type  const& this_type;

        template <class T>
        auto operator()(ast::type::Primitive<T> const& primitive) -> hir::Type {
            return {
                .value = primitive,
                .source_view = this_type.source_view
            };
        }

        auto operator()(ast::type::Wildcard const&) -> hir::Type {
            return {
                .value = hir::type::Wildcard {},
                .source_view = this_type.source_view
            };
        }

        auto operator()(ast::type::Typename const& name) -> hir::Type {
            return {
                .value = hir::type::Typename { context.lower(name.identifier) },
                .source_view = this_type.source_view
            };
        }

        auto operator()(ast::type::Tuple const& tuple) -> hir::Type {
            return {
                .value = hir::type::Tuple {
                    .types = bu::map(context.lower())(tuple.types)
                },
                .source_view = this_type.source_view
            };
        }

        auto operator()(ast::type::Array const& array) -> hir::Type {
            return {
                .value = hir::type::Array {
                    .element_type = context.lower(array.element_type),
                    .length = context.lower(array.length)
                },
                .source_view = this_type.source_view
            };
        }

        auto operator()(ast::type::Slice const& slice) -> hir::Type {
            return {
                .value = hir::type::Slice {
                    .element_type = context.lower(slice.element_type)
                },
                .source_view = this_type.source_view
            };
        }

        auto operator()(ast::type::Function const& function) -> hir::Type {
            return {
                .value = hir::type::Function {
                    .argument_types = bu::map(context.lower())(function.argument_types),
                    .return_type = context.lower(function.return_type)
                },
                .source_view = this_type.source_view
            };
        }

        auto operator()(ast::type::Type_of const& type_of) -> hir::Type {
            return {
                .value = hir::type::Type_of {
                    .expression = context.lower(type_of.expression)
                },
                .source_view = this_type.source_view
            };
        }

        auto operator()(ast::type::Reference const& reference) -> hir::Type {
            return {
                .value = hir::type::Reference {
                    .type = context.lower(reference.type),
                    .mutability = reference.mutability
                },
                .source_view = this_type.source_view
            };
        }

        auto operator()(ast::type::Pointer const& pointer) -> hir::Type {
            return {
                .value = hir::type::Pointer {
                    .type = context.lower(pointer.type),
                    .mutability = pointer.mutability
                },
                .source_view = this_type.source_view
            };
        }

        auto operator()(ast::type::Instance_of const& instance_of) -> hir::Type {
            if (context.current_function_implicit_template_parameters) {
                // Within a function's parameter list or return type, inst types
                // are converted into references to implicit template parameters.

                auto const tag = context.fresh_tag();

                context.current_function_implicit_template_parameters->emplace_back(
                    bu::map(context.lower())(instance_of.classes),
                    hir::Implicit_template_parameter::Tag { tag }
                );

                return {
                    .value = hir::type::Implicit_parameter_reference { .tag = tag },
                    .source_view = this_type.source_view
                };
            }
            else if (context.is_within_function()) {
                // Within a function body, inst types are simply used for constraint collection.

                return {
                    .value = hir::type::Instance_of {
                        .classes = bu::map(context.lower())(instance_of.classes)
                    },
                    .source_view = this_type.source_view
                };
            }
            else {
                context.diagnostics.emit_simple_error({
                    .erroneous_view = this_type.source_view,
                    .source = context.source,
                    .message_format = "'inst' types are only usable within functions"
                });
            }
        }

        auto operator()(ast::type::Template_application const& application) -> hir::Type {
            return {
                .value = hir::type::Template_application {
                    .arguments = bu::map(context.lower())(application.arguments),
                    .name      = context.lower(application.name)
                },
                .source_view = this_type.source_view
            };
        }
    };

}


auto Lowering_context::lower(ast::Type const& type) -> hir::Type {
    return std::visit(
        Type_lowering_visitor {
            .context = *this,
            .this_type = type
        },
        type.value
    );
}