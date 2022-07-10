#pragma once

#include "bu/utilities.hpp"
#include "ast/ast.hpp"
#include "hir/hir.hpp"


namespace ast {

    auto lower(Module&&) -> hir::Module;

}