#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Type_resolution_visitor {
        compiler::Resolution_context& context;

        auto recurse(ast::Type& type) -> ir::Type {
            return compiler::resolve_type(type, context);
        }

        auto recurse() {
            return [&](ast::Type& type) -> ir::Type {
                return recurse(type);
            };
        }


        template <class T>
        auto operator()(ast::type::Primitive<T>&) -> ir::Type {
            return { .value = ir::type::Primitive<T> {}, .size = sizeof(T) };
        }

        auto operator()(ast::type::Typename& name) -> ir::Type {
            return std::visit(
                bu::Overload {
                    [](std::monostate) -> ir::Type { bu::unimplemented(); },
                    [](ast::definition::Struct*) -> ir::Type { bu::unimplemented(); },
                    [](ast::definition::Data*) -> ir::Type { bu::unimplemented(); }
                },
                context.find_one_of<
                    &compiler::Namespace::struct_definitions,
                    &compiler::Namespace::data_definitions
                >(name.identifier)
            );
        }

        auto operator()(ast::type::Template_instantiation& instantiation) -> ir::Type {
            return std::visit(
                bu::Overload {
                    [](std::monostate) -> ir::Type {
                        bu::unimplemented();
                    },
                    [](ast::definition::Struct_template*) -> ir::Type {
                        bu::unimplemented();
                    },
                    [](ast::definition::Data_template*) -> ir::Type {
                        bu::unimplemented();
                    },
                    [](ast::definition::Alias_template*) -> ir::Type {
                        bu::unimplemented();
                    }
                },
                context.find_one_of<
                    &compiler::Namespace::struct_template_definitions,
                    &compiler::Namespace::data_template_definitions,
                    &compiler::Namespace::alias_template_definitions
                >(instantiation.name)
            );
        }

        auto operator()(ast::type::Tuple& tuple) -> ir::Type {
            ir::type::Tuple ir_tuple;
            bu::U16         size = 0;

            ir_tuple.types.reserve(tuple.types.size());

            for (auto& type : tuple.types) {
                ir_tuple.types.push_back(recurse(type));
                size += ir_tuple.types.back().size;
            }

            return { .value = std::move(ir_tuple), .size = size };
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
            return { .value = ir::type::Pointer { recurse(pointer.type) }, .size = sizeof(std::byte*) };
        }

        auto operator()(ast::type::Reference& reference) -> ir::Type {
            return { .value = ir::type::Reference { recurse(reference.type) }, .size = sizeof(std::byte*) };
        }

        auto operator()(auto&) -> ir::Type {
            bu::unimplemented();
        }
    };

}


auto compiler::resolve_type(ast::Type& type, Resolution_context& context) -> ir::Type {
    return std::visit(Type_resolution_visitor { context }, type.value);
}