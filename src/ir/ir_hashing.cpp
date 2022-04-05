#include "bu/utilities.hpp"
#include "ir.hpp"


namespace {

    template <class>
    struct IR_node_tag;

    // We give a unique tag for each IR node type to decrease the chance of collision

    template <> struct IR_node_tag<ir::Expression> : std::integral_constant<bu::Usize, __LINE__> {};
    template <> struct IR_node_tag<ir::Type      > : std::integral_constant<bu::Usize, __LINE__> {};


    template <class T>
    struct Hash_visitor_base {
        bu::Usize variant_index;

        auto hash(auto const&... args) const -> bu::Usize {
            return bu::hash_combine_with_seed(variant_index, IR_node_tag<T>::value, args...);
        }
    };


    struct Expression_hash_visitor : Hash_visitor_base<ir::Expression> {
        template <class T>
        auto operator()(ir::expression::Literal<T> const& literal) -> bu::Usize {
            return hash(literal.value);
        }
        auto operator()(ir::expression::Literal<lexer::String> const& literal) -> bu::Usize {
            return hash(literal.value.view());
        }

        auto operator()(ir::expression::Array_literal const& array) -> bu::Usize {
            return hash(array.elements);
        }

        auto operator()(ir::expression::Tuple const& tuple) -> bu::Usize {
            return hash(tuple.expressions);
        }

        auto operator()(ir::expression::Invocation const& invocation) -> bu::Usize {
            return hash(invocation.invocable, invocation.arguments);
        }

        auto operator()(ir::expression::Struct_initializer const& init) -> bu::Usize {
            return hash(init.type, init.member_initializers);
        }

        auto operator()(ir::expression::Local_variable const& variable) -> bu::Usize {
            return hash(variable.frame_offset);
        }

        auto operator()(ir::expression::Reference const& reference) -> bu::Usize {
            return hash(reference.frame_offset);
        }

        auto operator()(ir::expression::Function_reference const& function) -> bu::Usize {
            return hash(function.definition->name);
        }

        auto operator()(ir::expression::Member_access const& member_access) -> bu::Usize {
            return hash(member_access.expression, member_access.offset.get());
        }

        auto operator()(ir::expression::Let_binding const& let_binding) -> bu::Usize {
            return hash(let_binding.initializer);
        }

        auto operator()(ir::expression::Infinite_loop const& loop) -> bu::Usize {
            return hash(loop.body);
        }

        auto operator()(ir::expression::While_loop const& loop) -> bu::Usize {
            return hash(loop.condition, loop.body);
        }

        auto operator()(ir::expression::Conditional const& conditional) -> bu::Usize {
            return hash(conditional.condition, conditional.true_branch, conditional.false_branch);
        }

        auto operator()(ir::expression::Type_cast const& cast) -> bu::Usize {
            return hash(cast.expression, cast.type);
        }

        auto operator()(ir::expression::Compound const& compound) -> bu::Usize {
            return hash(compound.side_effects, compound.result);
        }
    };


    struct Type_hash_visitor : Hash_visitor_base<ir::Type> {
        template <class T>
        auto operator()(ir::type::Primitive<T> const&) -> bu::Usize {
            return hash();
        }

        auto operator()(ir::type::Array const& array) -> bu::Usize {
            return hash(array.element_type, array.length);
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
            return hash(function.return_type, function.parameter_types);
        }

        auto operator()(ir::type::Pointer const& pointer) -> bu::Usize {
            return hash(pointer.type, pointer.mut);
        }

        auto operator()(ir::type::Reference const& reference) -> bu::Usize {
            // *T will differ from &T because of the different variant index
            return operator()(reference.pointer);
        }

        auto operator()(ir::type::Tuple const& tuple) -> bu::Usize {
            return hash(tuple.types);
        }

        auto operator()(ir::type::User_defined_data const& udd) -> bu::Usize {
            return hash(udd.data->name);
        }

        auto operator()(ir::type::User_defined_struct const& uds) -> bu::Usize {
            return hash(uds.structure->name);
        }
    };

}


auto std::hash<ir::Expression>::operator()(ir::Expression const& expression) const -> bu::Usize {
    return bu::hash_combine_with_seed(
        std::visit(Expression_hash_visitor { { expression.value.index() } }, expression.value),
        expression.type
    );
}

auto std::hash<ir::Type>::operator()(ir::Type const& type) const -> bu::Usize {
    return std::visit(Type_hash_visitor { { type.value.index() } }, type.value);
}