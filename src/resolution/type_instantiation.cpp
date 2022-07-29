#include "bu/utilities.hpp"
#include "resolution_internals.hpp"


namespace {

    struct Instantiation_visitor {
        resolution::Context              & context;
        mir::Template_parameter_set const& parameters;

        auto operator()(auto const&) -> bu::Wrapper<mir::Type> {
            bu::todo();
        }
    };

}


auto resolution::Context::instantiate(mir::type::For_all const& for_all)
    -> bu::Wrapper<mir::Type>
{
    return std::visit(
        Instantiation_visitor {
            .context    = *this,
            .parameters = for_all.parameters
        },
        for_all.body->value
    );
}