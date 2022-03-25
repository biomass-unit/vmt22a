#include "bu/utilities.hpp"
#include "ast/ast_formatting.hpp"
#include "ir_formatting.hpp"


namespace {

    struct Visitor_base {
        std::format_context::iterator out;

        auto format(std::string_view fmt, auto const&... args) {
            return std::vformat_to(out, fmt, std::make_format_args(args...));
        }
    };


    struct Expression_format_visitor : Visitor_base {
        template <class T>
        auto operator()(ir::expression::Literal<T> const& literal) {
            return format("{}", literal.value);
        }

        auto operator()(ir::expression::Let_binding const& binding) {
            return format("let {}: {} = {}", binding.pattern, binding.type, binding.initializer);
        }

        auto operator()(ir::expression::Local_variable const& variable) {
            return format("(Local {})", variable.type);
        }

        auto operator()(ir::expression::Compound const& compound) {
            format("{{ ");
            for (auto const& expression : compound.side_effects) {
                format("{}; ", expression);
            }
            return format("{} }}", compound.result);
        }
    };

    struct Type_format_visitor : Visitor_base {
        auto operator()(ir::type::Integer   const&) { return format("Int");    }
        auto operator()(ir::type::Floating  const&) { return format("Float");  }
        auto operator()(ir::type::Character const&) { return format("Char");   }
        auto operator()(ir::type::Boolean   const&) { return format("Bool");   }
        auto operator()(ir::type::String    const&) { return format("String"); }

        auto operator()(ir::type::Tuple const& tuple) {
            return format("({})", tuple.types);
        }

        auto operator()(ir::type::Function const& function) {
            return format("fn({}): {}", function.parameter_types, function.return_type);
        }

        auto operator()(ir::type::Array const& array) {
            return format("[{}; {}]", array.element_type, array.length);
        }

        auto operator()(ir::type::List const& list) {
            return format("[{}]", list.element_type);
        }

        auto operator()(ir::type::Reference const& reference) {
            return format(reference.mut ? "&mut {}" : "&{}", reference.type);
        }

        auto operator()(ir::type::Pointer const& pointer) {
            return format(pointer.mut ? "*mut {}" : "*{}", pointer.type);
        }

        auto operator()(ir::type::User_defined_struct const& ud) {
            return format("{}", ud.structure->name);
        }

        auto operator()(ir::type::User_defined_data const& ud) {
            return format("{}", ud.data->name);
        }
    };

}


DEFINE_FORMATTER_FOR(ir::Expression) {
    std::format_to(context.out(), "(");
    std::visit(Expression_format_visitor { { context.out() } }, value.value);
    return std::format_to(context.out(), ": {})", value.type);
}

DEFINE_FORMATTER_FOR(ir::Type) {
    std::format_to(context.out(), "(");
    std::visit(Type_format_visitor { { context.out() } }, value.value);
    return std::format_to(context.out(), ", size {})", value.size);
}