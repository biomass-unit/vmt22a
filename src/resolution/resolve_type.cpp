#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Type_resolution_visitor {
        resolution::Context& context;
        hir::Type     const& this_type;

        auto operator()(hir::type::Integer const&) -> mir::Type::Variant {
            return mir::type::Integer::i64;
        }

        auto operator()(hir::type::Array const& array) -> mir::Type::Variant {
            return mir::type::Array {
                .element_type = context.resolve(array.element_type),
                .length       = context.resolve(array.length)
            };
        }

        template <class T>
        auto operator()(T const&) -> mir::Type::Variant {
            bu::abort(typeid(T).name());
        }
    };

}


auto resolution::Context::resolve(hir::Type const& type) -> mir::Type {
    return {
        .value = std::visit(
            Type_resolution_visitor {
                .context   = *this,
                .this_type = type
            },
            type.value
        ),
        .source_view = type.source_view
    };
}