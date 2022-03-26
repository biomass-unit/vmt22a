#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Type_resolution_visitor {
        compiler::Resolution_context& context;

        auto recurse(ast::Type& type) {
            return compiler::resolve_type(type, context);
        }
        auto recurse() {
            return [this](ast::Type& type) {
                return recurse(type);
            };
        }


        template <class T>
        auto operator()(ast::type::Primitive<T>&) -> ir::Type {
            return {
                .value      = ir::type::Primitive<T> {},
                .size       = sizeof(T),
                .is_trivial = true
            };
        }

        auto operator()(ast::type::Typename& name) -> ir::Type {
            if (auto const upper = context.find_upper(name.identifier)) {
                bu::unimplemented();
            }
            else {
                bu::unimplemented();
            }
        }

        auto operator()(ast::type::Template_instantiation& instantiation) -> ir::Type {
            if (auto const upper = context.find_upper(instantiation.name)) {
                bu::unimplemented();
            }
            else {
                bu::unimplemented();
            }
        }

        auto operator()(ast::type::Tuple& tuple) -> ir::Type {
            ir::type::Tuple ir_tuple;
            bu::U16         size       = 0;
            bool            is_trivial = true;

            ir_tuple.types.reserve(tuple.types.size());

            for (auto& type : tuple.types) {
                auto ir_type = recurse(type);

                size += ir_type.size;
                if (!ir_type.is_trivial) {
                    is_trivial = false;
                }

                ir_tuple.types.push_back(std::move(ir_type));
            }

            return {
                .value      = std::move(ir_tuple),
                .size       = size,
                .is_trivial = is_trivial
            };
        }

        auto operator()(ast::type::Array& array) -> ir::Type {
            if (auto* const length = std::get_if<ast::expression::Literal<bu::Isize>>(&array.length->value)) {
                // do meta evaluation later, the length shouldn't be restricted to a literal

                assert(length->value >= 0);

                auto type = ir::type::Array {
                    .element_type = recurse(array.element_type),
                    .length       = static_cast<bu::Usize>(length->value)
                };
                auto size = type.length * type.element_type->size;

                return { .value = std::move(type), .size = static_cast<bu::U16>(size) };
            }
            else {
                bu::abort("non-literal array lengths are not supported yet");
            }
        }

        auto operator()(ast::type::Pointer& pointer) -> ir::Type {
            return {
                .value      = ir::type::Pointer { recurse(pointer.type) },
                .size       = sizeof(std::byte*),
                .is_trivial = true
            };
        }

        auto operator()(ast::type::Reference& reference) -> ir::Type {
            return {
                .value      = ir::type::Reference { recurse(reference.type) },
                .size       = sizeof(std::byte*),
                .is_trivial = true
            };
        }

        auto operator()(ast::type::Type_of& type_of) -> ir::Type {
            if (std::holds_alternative<ast::expression::Let_binding>(type_of.expression->value)) {
                bu::abort("can not take type of let binding");
            }

            bool const is_unevaluated = std::exchange(context.is_unevaluated, true);
            auto expression = compiler::resolve_expression(type_of.expression, context);
            context.is_unevaluated = is_unevaluated;

            return std::move(*expression.type);
        }

        auto operator()(auto&) -> ir::Type {
            bu::unimplemented();
        }
    };

}


auto compiler::resolve_type(ast::Type& type, Resolution_context& context) -> ir::Type {
    return std::visit(Type_resolution_visitor { context }, type.value);
}