#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Definition_visitor {
        resolution::Resolution_context& context;

        auto operator()(resolution::Binding*) -> void {
            bu::unreachable();
        }

        auto operator()(resolution::Function_definition) -> void {
            bu::unimplemented();
        }

        auto operator()(resolution::Function_template_definition) -> void {
            bu::unimplemented();
        }

        auto operator()(resolution::Struct_definition) -> void {
            bu::unimplemented();
        }

        auto operator()(resolution::Struct_template_definition) -> void {
            bu::unimplemented();
        }

        auto operator()(resolution::Data_definition) -> void {
            bu::unimplemented();
        }

        auto operator()(resolution::Data_template_definition) -> void {
            bu::unimplemented();
        }

        auto operator()(resolution::Alias_definition) -> void {
            bu::unimplemented();
        }

        auto operator()(resolution::Alias_template_definition) -> void {
            bu::unimplemented();
        }

        auto operator()(resolution::Typeclass_definition) -> void {
            bu::unimplemented();
        }

        auto operator()(resolution::Typeclass_template_definition) -> void {
            bu::unimplemented();
        }
    };

}


auto resolution::resolve_lower(Lower_variant const lower, Resolution_context& context) -> void {
    std::visit(Definition_visitor { context }, lower);
}

auto resolution::resolve_upper(Upper_variant const upper, Resolution_context& context) -> void {
    std::visit(Definition_visitor { context }, upper);
}