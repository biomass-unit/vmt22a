#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Definition_visitor {
        compiler::Resolution_context& context;
        //bu::Flatmap<lexer::Identifier, bu::Wrapper<ir::Type>>* template_type_arguments = nullptr;


        auto operator()(ast::definition::Function*) -> void {
            bu::unimplemented();
        }

        auto operator()(ast::definition::Struct*) -> void {
            bu::unimplemented();
        }

        auto operator()(ast::definition::Data*) -> void {
            bu::unimplemented();
        }

        auto operator()(ast::definition::Alias*) -> void {
            bu::unimplemented();
        }

        auto operator()(ast::definition::Typeclass*) -> void {
            bu::unimplemented();
        }

        auto operator()(ast::definition::Namespace*) -> void {
            bu::unimplemented();
        }

        auto operator()(ast::definition::Implementation*) -> void {
            bu::unimplemented();
        }

        auto operator()(ast::definition::Instantiation*) -> void {
            bu::unimplemented();
        }

        template <class Definition>
        auto operator()(ast::definition::Template_definition<Definition>*) -> void {
            bu::unimplemented();
        }
    };

}


auto compiler::resolve_lower(Lower_variant const lower, Resolution_context& context) -> void {
    std::visit(Definition_visitor { context }, lower);
}

auto compiler::resolve_upper(Upper_variant const upper, Resolution_context& context) -> void {
    std::visit(Definition_visitor { context }, upper);
}