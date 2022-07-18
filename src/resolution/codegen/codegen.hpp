#pragma once

#include "bu/utilities.hpp"
#include "lir/lir.hpp"
#include "resolution/resolution.hpp"


namespace resolution {

    auto codegen(lir::Module&&) -> Module::Code;

}