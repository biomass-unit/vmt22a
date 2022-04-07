#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Type_resolution_visitor {
        resolution::Resolution_context& context;
        ast::Type                     & this_type;

        auto recurse(ast::Type& type) -> bu::Wrapper<ir::Type> {
            return resolution::resolve_type(type, context);
        }
        auto recurse() {
            return [this](ast::Type& type) -> bu::Wrapper<ir::Type> {
                return recurse(type);
            };
        }

        auto error(std::string_view message) -> std::runtime_error {
            return context.error({
                .erroneous_view = this_type.source_view,
                .message        = message
            });
        }


        template <class T>
        auto operator()(ast::type::Primitive<T>&) -> bu::Wrapper<ir::Type> {
            return ir::type::dtl::make_primitive<T>;
        }

        auto operator()(ast::type::Typename& name) -> bu::Wrapper<ir::Type> {
            return context.new_find_type(
                name.identifier,
                this_type.source_view,
                std::nullopt
            );
        }

        auto operator()(ast::type::Template_instantiation& instantiation) -> bu::Wrapper<ir::Type> {
            return context.new_find_type(
                instantiation.name,
                this_type.source_view,
                instantiation.arguments
            );
        }

        auto operator()(ast::type::Tuple& tuple) -> bu::Wrapper<ir::Type> {
            ir::type::Tuple ir_tuple;
            ir::Size_type   size;
            bool            is_trivial = true;

            ir_tuple.types.reserve(tuple.types.size());

            for (auto& type : tuple.types) {
                bu::wrapper auto ir_type = recurse(type);

                size.safe_add(ir_type->size.get());

                if (!ir_type->is_trivial) {
                    is_trivial = false;
                }

                ir_tuple.types.push_back(ir_type);
            }

            return ir::Type {
                .value      = std::move(ir_tuple),
                .size       = size,
                .is_trivial = is_trivial
            };
        }

        auto operator()(ast::type::Array& array) -> bu::Wrapper<ir::Type> {
            if (auto* const length = std::get_if<ast::expression::Literal<bu::Isize>>(&array.length->value)) {
                // do meta evaluation later, the length shouldn't be restricted to a literal

                assert(length->value >= 0);

                auto type = ir::type::Array {
                    .element_type = recurse(array.element_type),
                    .length       = static_cast<bu::Usize>(length->value)
                };
                auto const size = type.element_type->size.copy().safe_mul(type.length);

                return ir::Type {
                    .value = std::move(type),
                    .size  = size
                };
            }
            else {
                bu::abort("non-literal array lengths are not supported yet");
            }
        }

        auto resolve_pointer(auto& pointer) -> ir::type::Pointer {
            return {
                .type = recurse(pointer.type),
                .mut  = context.resolve_mutability(pointer.mutability)
            };
        }

        auto operator()(ast::type::Pointer& pointer) -> bu::Wrapper<ir::Type> {
            return ir::Type {
                .value      = resolve_pointer(pointer),
                .size       = ir::Size_type { bu::unchecked_tag, sizeof(std::byte*) },
                .is_trivial = true
            };
        }

        auto operator()(ast::type::Reference& reference) -> bu::Wrapper<ir::Type> {
            return ir::Type {
                .value      = ir::type::Reference { resolve_pointer(reference) },
                .size       = ir::Size_type { bu::unchecked_tag, sizeof(std::byte*) },
                .is_trivial = true
            };
        }

        auto operator()(ast::type::Type_of& type_of) -> bu::Wrapper<ir::Type> {
            auto child_context = context.make_child_context_with_new_scope();
            child_context.is_unevaluated = true;
            return resolution::resolve_expression(type_of.expression, child_context).type;
        }

        auto operator()(auto&) -> bu::Wrapper<ir::Type> {
            bu::unimplemented();
        }
    };

}


auto resolution::resolve_type(ast::Type& type, Resolution_context& context) -> bu::Wrapper<ir::Type> {
    return std::visit(Type_resolution_visitor { context, type }, type.value);
}