#include "bu/utilities.hpp"
#include "tst.hpp"


namespace {

    template <class>
    struct TST_node_tag;

    // We give a unique tag for each TST node type to decrease the chance of collision

    template <> struct TST_node_tag<tst::Expression> : std::integral_constant<bu::Usize, bu::get_unique_seed()> {};
    template <> struct TST_node_tag<tst::Type      > : std::integral_constant<bu::Usize, bu::get_unique_seed()> {};


    template <class T>
    struct Hash_visitor_base {
        bu::Usize variant_index;

        auto hash(auto const&... args) const -> bu::Usize {
            return bu::hash_combine_with_seed(variant_index, TST_node_tag<T>::value, args...);
        }
    };


    struct Expression_hash_visitor : Hash_visitor_base<tst::Expression> {
        template <class T>
        auto operator()(tst::expression::Literal<T> const& literal) -> bu::Usize {
            return hash(literal.value);
        }

        auto operator()(tst::expression::String const& literal) -> bu::Usize {
            return hash(literal.value.view());
        }

        auto operator()(tst::expression::Array_literal const& array) -> bu::Usize {
            return hash(array.elements);
        }

        auto operator()(tst::expression::Tuple const& tuple) -> bu::Usize {
            return hash(tuple.expressions);
        }

        auto operator()(tst::expression::Invocation const& invocation) -> bu::Usize {
            return hash(invocation.invocable, invocation.arguments);
        }

        auto operator()(tst::expression::Struct_initializer const& init) -> bu::Usize {
            return hash(init.type, init.member_initializers);
        }

        auto operator()(tst::expression::Local_variable const& variable) -> bu::Usize {
            return hash(variable.frame_offset);
        }

        auto operator()(tst::expression::Reference const& reference) -> bu::Usize {
            return hash(reference.frame_offset);
        }

        auto operator()(tst::expression::Function_reference const& function) -> bu::Usize {
            return hash(function.definition->name);
        }

        auto operator()(tst::expression::Enum_constructor_reference const& reference) -> bu::Usize {
            return hash(reference.enumeration->enum_type, reference.constructor->tag);
        }

        auto operator()(tst::expression::Struct_member_access const& access) -> bu::Usize {
            return hash(access.expression, access.member_name);
        }

        auto operator()(tst::expression::Tuple_member_access const& access) -> bu::Usize {
            return hash(access.expression, access.member_index);
        }

        auto operator()(tst::expression::Let_binding const& let_binding) -> bu::Usize {
            return hash(let_binding.initializer);
        }

        auto operator()(tst::expression::Infinite_loop const& loop) -> bu::Usize {
            return hash(loop.body);
        }

        auto operator()(tst::expression::While_loop const& loop) -> bu::Usize {
            return hash(loop.condition, loop.body);
        }

        auto operator()(tst::expression::Conditional const& conditional) -> bu::Usize {
            return hash(conditional.condition, conditional.true_branch, conditional.false_branch);
        }

        auto operator()(tst::expression::Type_cast const& cast) -> bu::Usize {
            return hash(cast.expression, cast.type);
        }

        auto operator()(tst::expression::Compound const& compound) -> bu::Usize {
            return hash(compound.side_effects, compound.result);
        }
    };


    struct Type_hash_visitor : Hash_visitor_base<tst::Type> {
        template <class T>
        auto operator()(tst::type::Primitive<T> const&) -> bu::Usize {
            return hash();
        }

        auto operator()(tst::type::Array const& array) -> bu::Usize {
            return hash(array.element_type, array.length);
        }

        auto operator()(tst::type::Slice const& list) -> bu::Usize {
            return operator()(
                tst::type::Array {
                    .element_type = list.element_type,
                    .length       = std::numeric_limits<bu::Usize>::max()
                }
            );
        }

        auto operator()(tst::type::Function const& function) -> bu::Usize {
            return hash(function.return_type, function.parameter_types);
        }

        auto operator()(tst::type::Pointer const& pointer) -> bu::Usize {
            return hash(pointer.type, pointer.mut);
        }

        auto operator()(tst::type::Reference const& reference) -> bu::Usize {
            // *T will differ from &T because of the different variant index
            return operator()(reference.pointer);
        }

        auto operator()(tst::type::Tuple const& tuple) -> bu::Usize {
            return hash(tuple.types);
        }

        auto operator()(tst::type::User_defined_enum const& ude) -> bu::Usize {
            return hash(ude.enumeration->name);
        }

        auto operator()(tst::type::User_defined_struct const& uds) -> bu::Usize {
            return hash(uds.structure->name);
        }
    };

}


auto tst::Expression::hash() const -> bu::Usize {
    return bu::hash_combine_with_seed(std::visit(Expression_hash_visitor { { value.index() } }, value), type);
}

auto tst::Type::hash() const -> bu::Usize {
    return std::visit(Type_hash_visitor { { value.index() } }, value);
}

auto tst::Template_argument_set::hash() const -> bu::Usize {
    return bu::hash_combine_with_seed(
        bu::get_unique_seed(),
        expression_arguments,
        type_arguments,
        mutability_arguments
    );
}