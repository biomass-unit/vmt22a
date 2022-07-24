#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Type_resolution_visitor {
        resolution::Context  & context;
        resolution::Scope    & scope;
        resolution::Namespace& space;
        hir::Type            & this_type;

        auto recurse(hir::Type& type, resolution::Scope* const new_scope = nullptr)
            -> bu::Wrapper<mir::Type>
        {
            return std::visit(
                Type_resolution_visitor {
                    context,
                    new_scope ? *new_scope : scope,
                    space,
                    this_type
                },
                type.value
            );
        }


        auto operator()(hir::type::Integer&) -> bu::Wrapper<mir::Type> {
            return mir::Type {
                .value       = mir::type::Integer::i64,
                .source_view = this_type.source_view
            };
        }
        auto operator()(hir::type::String&) -> bu::Wrapper<mir::Type> {
            return mir::Type {
                .value       = mir::type::String {},
                .source_view = this_type.source_view
            };
        }
        auto operator()(hir::type::Floating&) -> bu::Wrapper<mir::Type> {
            return mir::Type {
                .value       = mir::type::Floating {},
                .source_view = this_type.source_view
            };
        }
        auto operator()(hir::type::Character&) -> bu::Wrapper<mir::Type> {
            return mir::Type {
                .value       = mir::type::Character {},
                .source_view = this_type.source_view
            };
        }
        auto operator()(hir::type::Boolean&) -> bu::Wrapper<mir::Type> {
            return mir::Type {
                .value       = mir::type::Boolean {},
                .source_view = this_type.source_view
            };
        }

        auto operator()(hir::type::Array& array) -> bu::Wrapper<mir::Type> {
            bu::wrapper auto const element_type = recurse(*array.element_type);
            auto [constraints, length] = context.resolve_expression(*array.length, scope, space);
            constraints.equality_constraints.emplace_back(length.type, context.size_type);
            context.unify(constraints);

            return mir::Type {
                .value = mir::type::Array {
                    .element_type = element_type,
                    .length       = std::move(length)
                },
                .source_view = this_type.source_view
            };
        }

        auto operator()(hir::type::Type_of& type_of) -> bu::Wrapper<mir::Type> {
            auto child_scope = scope.make_child();
            auto [constraints, expression] = context.resolve_expression(*type_of.expression, child_scope, space);
            context.unify(constraints);
            return expression.type;
        }

        auto operator()(hir::type::Typename& name) -> bu::Wrapper<mir::Type> {
            if (auto type = context.find_type(scope, space, name.identifier)) {
                return std::visit(bu::Overload {
                    [this](bu::Wrapper<resolution::Struct_info> const info) -> bu::Wrapper<mir::Type> {
                        return mir::Type {
                            .value       = mir::type::Structure { info },
                            .source_view = this_type.source_view
                        };
                    },
                    [](auto&) -> bu::Wrapper<mir::Type> {
                        bu::todo();
                    }
                }, *type);
            }
            else {
                context.error(bu::get(this_type.source_view), { "Unrecognized typename" }); // FIX
            }
        }

        template <class T>
        auto operator()(T&) -> bu::Wrapper<mir::Type> {
            bu::abort(typeid(T).name());
        }
    };

}


auto resolution::Context::resolve_type(hir::Type& type, Scope& scope, Namespace& space)
    -> bu::Wrapper<mir::Type>
{
    return Type_resolution_visitor {
        .context   = *this,
        .scope     = scope,
        .space     = space,
        .this_type = type
    }.recurse(type);
}