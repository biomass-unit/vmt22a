#include "bu/utilities.hpp"
#include "resolution.hpp"

#include "lexer/lexer.hpp"
#include "parser/parser.hpp"


namespace {

    struct Resolution_context {};


    auto resolve_type(ast::Type&, Resolution_context&) -> ir::Type;

    struct Type_resolution_visitor {
        Resolution_context& context;

        auto recurse(ast::Type& type) -> ir::Type {
            return resolve_type(type, context);
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
            if (auto* const length = std::get_if<ast::Literal<bu::Isize>>(&array.length->value)) {
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

    auto resolve_type(ast::Type& type, Resolution_context& context) -> ir::Type {
        return std::visit(Type_resolution_visitor { context }, type.value);
    }


    auto handle_imports(ast::Module& module) -> void {
        auto project_root = std::filesystem::current_path() / "sample-project"; // for debugging purposes, programmatically find path later

        std::vector<ast::Module> modules;
        modules.reserve(module.imports.size());

        for (auto& import : module.imports) {
            auto directory = project_root;

            for (auto& component : import.path | std::views::take(import.path.size() - 1)) {
                directory /= component.view();
            }

            auto path = directory / (std::string { import.path.back().view() } + ".vmt"); // fix later
            modules.push_back(parser::parse(lexer::lex(bu::Source { path.string() })));
        }

        {
            // add stuff to the namespace
        }
    }

}


auto compiler::resolve(ast::Module&& module) -> ir::Program {
    handle_imports(module);

    // vvv Release all memory used by the AST

    bu::Wrapper<ast::Expression>::release_wrapped_memory();
    bu::Wrapper<ast::Type      >::release_wrapped_memory();
    bu::Wrapper<ast::Pattern   >::release_wrapped_memory();

    // ^^^ Fix, use RAII or find wrapped types programmatically ^^^

    return {};

    // Perform imports, scope resolution, type
    // checking, turn names into actual references, etc.
}