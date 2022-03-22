#include "bu/utilities.hpp"
#include "codegen_internals.hpp"
#include "ast/ast_formatting.hpp"

using namespace bu::literals;


namespace {

    using namespace compiler;

    auto recurse_with(compiler::Codegen_context& context) noexcept {
        return [&](ir::Type& type) {
            return compiler::size_of(type, context);
        };
    }


    /*auto size_of_data(ast::definition::Data& type, Codegen_context& context) -> bu::Usize {
        auto const size_of_ctor = [&](auto& constructor) -> bu::Usize {
            return constructor.type.transform(recurse_with(context)).value_or(0);
        };
        return type.constructors.empty()
            ? 1_uz
            : 1_uz + std::ranges::max(type.constructors | std::views::transform(size_of_ctor));
        // +1 for the tag byte
    }

    auto size_of_struct(ast::definition::Struct& type, Codegen_context& context) -> bu::Usize {
        return std::transform_reduce(
            type.members.begin(),
            type.members.end(),
            0_uz,
            std::plus<bu::Usize> {},
            bu::compose(recurse_with(context), &ast::definition::Struct::Member::type)
        );
    }

    auto size_of_user_defined(ast::Qualified_name& name, Codegen_context& context) -> bu::Usize {
        return std::visit(bu::Overload {
            [&](ast::definition::Struct* structure) {
                return structure->size.emplace(size_of_struct(*structure, context));
            },
            [&](ast::definition::Data* data) {
                return data->size.emplace(size_of_data(*data, context));
            },
            [&](ast::definition::Alias* alias) {
                return static_cast<bu::Usize>(compiler::size_of(alias->type, context));
            },
            [](ast::definition::Typeclass*)                       -> bu::Usize { bu::unimplemented(); },
            []<class T>(ast::definition::Template_definition<T>*) -> bu::Usize { bu::unimplemented(); },
            [](std::monostate)                                    -> bu::Usize { bu::unimplemented(); },
        }, context.space->find_upper(name));
    }*/


    struct Size_visitor {
        compiler::Codegen_context& context;
        ir::Type                 & this_type;

        auto recurse(ir::Type& type) {
            return recurse_with(context)(type);
        }


        template <class T>
        auto operator()(ir::type::Primitive<T>&) noexcept -> bu::Usize {
            return sizeof(T);
        }

        auto operator()(ir::type::Tuple& tuple) -> bu::Usize {
            return std::transform_reduce(
                tuple.types.begin(),
                tuple.types.end(),
                0_uz,
                std::plus {},
                recurse_with(context)
            );
        }

        auto operator()(ir::type::User_defined_data&) -> bu::Usize {
            bu::unimplemented();
        }

        auto operator()(ir::type::User_defined_struct&) -> bu::Usize {
            bu::unimplemented();
        }

        auto operator()(ir::type::Array& array) -> bu::Usize {
            return recurse(array.element_type) * array.length;
        }

        auto operator()(ir::type::List&) -> bu::Usize {
            bu::unimplemented();
        }

        auto operator()(ir::type::Function&) noexcept -> bu::Usize {
            return sizeof(vm::Jump_offset_type) + sizeof(std::byte*);
            // Function entry offset + capture pointer
        }

        auto operator()(bu::one_of<ir::type::Pointer, ir::type::Reference> auto&) noexcept -> bu::Usize {
            return sizeof(std::byte*);
        }
    };

}


auto compiler::size_of(ir::Type& type, Codegen_context& context) -> vm::Local_size_type {
    auto const size = std::visit(Size_visitor { context, type }, type.value);
    return type.size = static_cast<vm::Local_size_type>(size);
}


static_assert(std::is_same_v<vm::Local_size_type, decltype(ir::Type::size)>);