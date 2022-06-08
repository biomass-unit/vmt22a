#include "bu/utilities.hpp"
#include "typechecker.hpp"


namespace {

    struct Constraint {
        struct Equality {

        };

        struct Conformity {

        };

        std::variant<Equality, Conformity> value;
    };

}


auto typechecker::typecheck(ast::Module&&) -> Checked_program {
    bu::unimplemented();
}