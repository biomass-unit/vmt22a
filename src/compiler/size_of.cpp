#include "bu/utilities.hpp"
#include "size_of.hpp"
#include "ast/ast_formatting.hpp"

using namespace bu::literals;


namespace {

    auto recurse_with(ast::Namespace& space) noexcept {
        return [&](ast::Type& type) {
            return compiler::size_of(type, space);
        };
    }


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
        return std::transform_reduce(
            type.members.begin(),
            type.members.end(),
            0_uz,
            std::plus<bu::Usize> {},
            bu::compose(recurse_with(space), &ast::definition::Struct::Member::type)
        );
    }

    auto size_of_user_defined(ast::Qualified_name& name, ast::Namespace& space) -> bu::Usize {
        return std::visit(bu::Overload {
            [&](ast::definition::Struct* structure) {
                return structure->size.emplace(size_of_struct(*structure, space));
            },
            [&](ast::definition::Data* data) {
                return data->size.emplace(size_of_data(*data, space));
            },
            [&](ast::definition::Alias* alias) {
                return static_cast<bu::Usize>(compiler::size_of(alias->type, space));
            },
            [](ast::definition::Typeclass*)                       -> bu::Usize { bu::unimplemented(); },
            []<class T>(ast::definition::Template_definition<T>*) -> bu::Usize { bu::unimplemented(); },
            [](std::monostate)                                    -> bu::Usize { bu::unimplemented(); },
        }, space.find_upper(name));
    }


    struct Size_visitor {
        ast::Namespace& space;
        ast::Type     & this_type;

        auto recurse(ast::Type& type) {
            return recurse_with(space)(type);
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
                recurse_with(space)
            );
        }

        auto operator()(ast::type::Typename& name) -> bu::Usize {
            return size_of_user_defined(name.identifier, space);
        }

        auto operator()(ast::type::Reference&) -> bu::Usize {
            return sizeof(std::byte*);
        }

        auto operator()(ast::type::Array& array) -> bu::Usize {
            return recurse(array.element_type) * array.length;
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