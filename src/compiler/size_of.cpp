#include "bu/utilities.hpp"
#include "codegen_internals.hpp"
#include "ast/ast_formatting.hpp"

using namespace bu::literals;


namespace {

    using namespace compiler;

    auto recurse_with(compiler::Codegen_context& context) noexcept {
        return [&](ast::Type& type) {
            return compiler::size_of(type, context);
        };
    }


    auto size_of_data(ast::definition::Data& type, Codegen_context& context) -> bu::Usize {
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
    }


    struct Size_visitor {
        compiler::Codegen_context& context;
        ast::Type                & this_type;

        auto recurse(ast::Type& type) {
            return recurse_with(context)(type);
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
                std::plus {},
                recurse_with(context)
            );
        }

        auto operator()(ast::type::Typename& name) -> bu::Usize {
            return size_of_user_defined(name.identifier, context);
        }

        auto operator()(ast::type::Array& array) -> bu::Usize {
            if (auto* length = std::get_if<ast::Literal<bu::Isize>>(&array.length->value)) {
                if (length->value >= 0) {
                    return recurse(array.element_type) * static_cast<bu::Usize>(length->value);
                }
                else {
                    bu::unimplemented();
                }
            }
            else {
                bu::unimplemented();
            }
        }

        auto operator()(ast::type::Function&) noexcept -> bu::Usize {
            return sizeof(vm::Jump_offset_type) + sizeof(std::byte*);
            // Function entry offset + capture pointer
        }

        auto operator()(ast::type::Type_of& type_of) -> bu::Usize {
            this_type = compiler::type_of(type_of.expression, context);
            return recurse(this_type);
            // fix?
        }

        auto operator()(bu::one_of<ast::type::Pointer, ast::type::Reference> auto&) noexcept -> bu::Usize {
            return sizeof(std::byte*);
        }

        auto operator()(auto&) -> bu::Usize {
            bu::abort(std::format("compiler::size_of has not been implemented for {}", this_type));
        }
    };

}


auto compiler::size_of(ast::Type& type, Codegen_context& context) -> vm::Local_size_type {
    if (!type.size) {
        auto const size = std::visit(Size_visitor { context, type }, type.value);
        type.size.emplace(static_cast<vm::Local_size_type>(size));
    }
    return *type.size;
}


static_assert(std::is_same_v<vm::Local_size_type, decltype(ast::Type::size)::value_type>);