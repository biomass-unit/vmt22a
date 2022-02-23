#include "bu/utilities.hpp"
#include "size_of.hpp"
#include "ast/ast_formatting.hpp"

using namespace bu::literals;


namespace {

    auto size_of_data(ast::definition::Data& type, ast::Namespace& space) -> bu::Usize {
        auto const size_of_ctor = [&](auto& constructor) -> bu::Usize {
            return constructor.type ? compiler::size_of(*constructor.type, space) : 0_uz;
        };
        return type.constructors.empty()
            ? 1_uz
            : 1_uz + std::ranges::max(type.constructors | std::views::transform(size_of_ctor));
        // +1 for the tag byte
    }

    auto size_of_struct(ast::definition::Struct& type, ast::Namespace& space) -> bu::Usize {
        auto const size_of_member = [&](auto& member) -> bu::Usize {
            return compiler::size_of(member.type, space);
        };
        auto range = type.members | std::views::transform(size_of_member);
        return std::accumulate(range.begin(), range.end(), 0_uz);
    }

    auto size_of_user_defined(lexer::Identifier const name, ast::Namespace& space) -> bu::Usize {
        static auto sizes =
            bu::vector_with_capacity<bu::Pair<lexer::Identifier, bu::Usize>>(16);

        if (auto it = std::ranges::find(sizes, name, bu::first); it != sizes.end()) {
            return it->second;
        }

        {
            auto it = std::ranges::find(space.data_definitions, name, &ast::definition::Data::name);
            if (it != space.data_definitions.end()) {
                return sizes.emplace_back(name, size_of_data(*it, space)).second;
            }
        }

        {
            auto it = std::ranges::find(space.struct_definitions, name, &ast::definition::Struct::name);
            if (it != space.struct_definitions.end()) {
                return sizes.emplace_back(name, size_of_struct(*it, space)).second;
            }
        }

        bu::abort(std::format("compiler::size_of: {} is not a type", name));
    }


    struct Size_visitor {
        ast::Namespace& space;
        ast::Type     & this_type;

        auto recurse() {
            return [&](ast::Type& type) {
                return compiler::size_of(type, space);
            };
        }

        template <class T>
        auto operator()(ast::type::Primitive<T>&) -> bu::Usize {
            return sizeof(T);
        }
        auto operator()(ast::type::Tuple& tuple) -> bu::Usize {
            return std::transform_reduce(
                tuple.types.begin(),
                tuple.types.end(),
                0_uz,
                std::plus<bu::Usize> {},
                recurse()
            );
        }
        auto operator()(ast::type::Typename& name) -> bu::Usize {
            return size_of_user_defined(name.identifier, space);
        }
        auto operator()(ast::type::Reference&) -> bu::Usize {
            return sizeof(std::byte*);
        }
        auto operator()(ast::type::Array& array) -> bu::Usize {
            return recurse()(array.element_type) * array.length;
        }
        auto operator()(auto&) -> bu::Usize {
            bu::abort(std::format("compiler::size_of has not been implemented for {}", this_type));
        }
    };

}


auto compiler::size_of(ast::Type& type, ast::Namespace& space) -> vm::Local_size_type {
    if (!type.size) {
        auto const size = std::visit(Size_visitor { space, type }, type.value);
        type.size.emplace(static_cast<vm::Local_size_type>(size));
    }
    return *type.size;
}


static_assert(std::is_same_v<vm::Local_size_type, decltype(ast::Type::size)::value_type>);