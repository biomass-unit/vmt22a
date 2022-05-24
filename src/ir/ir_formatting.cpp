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

        auto operator()(ir::expression::Array_literal const& array) {
            return std::format_to(out, "[{}]", array.elements);
        }

        auto operator()(ir::expression::Tuple const& tuple) {
            return format("({})", tuple.expressions);
        }

        auto operator()(ir::expression::Invocation const& invocation) {
            return format("{}({})", invocation.invocable, invocation.arguments);
        }

        auto operator()(ir::expression::Struct_initializer const& init) {
            return format("{} {{ {} }}", init.type, init.member_initializers);
        }

        auto operator()(ir::expression::Let_binding const& binding) {
            return format("bind {}", binding.initializer);
        }

        auto operator()(ir::expression::Local_variable const& variable) {
            return format("(Move offset {})", variable.frame_offset);
        }

        auto operator()(ir::expression::Function_reference const& function) {
            return format("{}", function.definition->name);
        }

        auto operator()(ir::expression::Enum_constructor_reference const& reference) {
            return format("{}::{}", reference.constructor->enum_type, reference.constructor->name);
        }

        auto operator()(ir::expression::Reference const& reference) {
            return format("(Ref offset {})", reference.frame_offset);
        }

        auto operator()(ir::expression::Member_access const& access) {
            return format("({}, mem offset {})", access.expression, access.offset);
        }

        auto operator()(ir::expression::Conditional const& conditional) {
            return format("if {} then {} else {}", conditional.condition, conditional.true_branch, conditional.false_branch);
        }

        auto operator()(ir::expression::Type_cast const& cast) {
            return format("({} as {})", cast.expression, cast.type);
        }

        auto operator()(ir::expression::Infinite_loop const& loop) {
            return format("loop {}", loop.body);
        }

        auto operator()(ir::expression::While_loop const& loop) {
            return format("while {} {}", loop.condition, loop.body);
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

        auto operator()(ir::type::Slice const& slice) {
            return format("[{}]", slice.element_type);
        }

        auto operator()(ir::type::Reference const& reference) {
            return format("&{}{}", reference.pointer.mut ? "mut " : "", reference.pointer.type);
        }

        auto operator()(ir::type::Pointer const& pointer) {
            return format("*{}{}", pointer.mut ? "mut " : "", pointer.type);
        }

        auto operator()(ir::type::User_defined_struct const& ud) {
            return format("{}", ud.structure->name);
        }

        auto operator()(ir::type::User_defined_enum const& ud) {
            return format("{}", ud.enumeration->name);
        }
    };

}


DEFINE_FORMATTER_FOR(ir::Expression) {
    return std::visit(Expression_format_visitor { { context.out() } }, value.value);
}

DEFINE_FORMATTER_FOR(ir::Type) {
    return std::visit(Type_format_visitor { { context.out() } }, value.value);
}