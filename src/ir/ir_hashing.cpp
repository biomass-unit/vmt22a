#include "bu/utilities.hpp"
#include "ir.hpp"


namespace {

    struct Expression_hash_visitor {
        auto operator()(auto const&) -> bu::Usize {
            bu::unimplemented();
        }
    };


    struct Type_hash_visitor {
        ir::Type const& this_type;

        template <class T>
        auto operator()(ir::type::Primitive<T> const&) -> bu::Usize {
            return this_type.value.index();
        }

        auto operator()(ir::type::Array const& array) -> bu::Usize {
            return bu::hash_combine(array.element_type, array.length);
        }

        auto operator()(ir::type::List const& list) -> bu::Usize {
            return operator()(
                ir::type::Array {
                    .element_type = list.element_type,
                    .length       = std::numeric_limits<bu::Usize>::max()
                }
            );
        }

        auto operator()(ir::type::Function const& function) -> bu::Usize {
            return bu::hash_combine(function.return_type, function.parameter_types);
        }

        auto operator()(ir::type::Pointer const& pointer) -> bu::Usize {
            return bu::hash_combine(pointer.type, pointer.mut);
        }

        auto operator()(ir::type::Reference const& reference) -> bu::Usize {
            return operator()(reference.pointer) + 1;
        }

        auto operator()(ir::type::Tuple const& tuple) -> bu::Usize {
            return bu::hash_combine(tuple.types);
        }

        auto operator()(ir::type::User_defined_data const& udd) -> bu::Usize {
            return bu::hash_combine(udd.data->name);
        }

        auto operator()(ir::type::User_defined_struct const& uds) -> bu::Usize {
            return bu::hash_combine(uds.structure->name);
        }
    };

}


auto std::hash<ir::Expression>::operator()(ir::Expression const& expression) const -> bu::Usize {
    return std::visit(Expression_hash_visitor {}, expression.value);
}

auto std::hash<ir::Type>::operator()(ir::Type const& type) const -> bu::Usize {
    return std::visit(Type_hash_visitor { type }, type.value);
}