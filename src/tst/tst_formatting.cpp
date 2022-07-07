#include "bu/utilities.hpp"
#include "tst_formatting.hpp"


namespace {

    struct Visitor_base {
        std::format_context::iterator out;

        auto format(std::string_view fmt, auto const&... args) {
            return std::vformat_to(out, fmt, std::make_format_args(args...));
        }
    };


    struct Expression_format_visitor : Visitor_base {
        template <class T>
        auto operator()(tst::expression::Literal<T> const& literal) {
            return format("{}", literal.value);
        }

        auto operator()(tst::expression::Array_literal const& array) {
            return std::format_to(out, "[{}]", array.elements);
        }

        auto operator()(tst::expression::Tuple const& tuple) {
            return format("({})", tuple.expressions);
        }

        auto operator()(tst::expression::Invocation const& invocation) {
            return format("{}({})", invocation.invocable, invocation.arguments);
        }

        auto operator()(tst::expression::Struct_initializer const& init) {
            return format("{} {{ {} }}", init.type, init.member_initializers);
        }

        auto operator()(tst::expression::Let_binding const& binding) {
            return format("bind {}", binding.initializer);
        }

        auto operator()(tst::expression::Local_variable const& variable) {
            return format("{}", variable.name);
        }

        auto operator()(tst::expression::Function_reference const& function) {
            return format("{}", function.definition->name);
        }

        auto operator()(tst::expression::Enum_constructor_reference const& reference) {
            return format("{}::{}", reference.enumeration->name, reference.constructor->name);
        }

        auto operator()(tst::expression::Reference const& reference) {
            return format("(Ref offset {})", reference.frame_offset);
        }

        auto operator()(tst::expression::Struct_member_access const& access) {
            return format("({}.{})", access.expression, access.member_name);
        }

        auto operator()(tst::expression::Tuple_member_access const& access) {
            return format("({}.{})", access.expression, access.member_index);
        }

        auto operator()(tst::expression::Conditional const& conditional) {
            return format("if {} then {} else {}", conditional.condition, conditional.true_branch, conditional.false_branch);
        }

        auto operator()(tst::expression::Type_cast const& cast) {
            return format("({} as {})", cast.expression, cast.type);
        }

        auto operator()(tst::expression::Infinite_loop const& loop) {
            return format("loop {}", loop.body);
        }

        auto operator()(tst::expression::While_loop const& loop) {
            return format("while {} {}", loop.condition, loop.body);
        }

        auto operator()(tst::expression::Compound const& compound) {
            format("{{ ");
            for (auto const& expression : compound.side_effects) {
                format("{}; ", expression);
            }
            return format("{} }}", compound.result);
        }
    };

    struct Type_format_visitor : Visitor_base {
        auto operator()(tst::type::Integer   const&) { return format("Int");    }
        auto operator()(tst::type::Floating  const&) { return format("Float");  }
        auto operator()(tst::type::Character const&) { return format("Char");   }
        auto operator()(tst::type::Boolean   const&) { return format("Bool");   }
        auto operator()(tst::type::String    const&) { return format("String"); }

        auto operator()(tst::type::Tuple const& tuple) {
            return format("({})", tuple.types);
        }

        auto operator()(tst::type::Function const& function) {
            return format("fn({}): {}", function.parameter_types, function.return_type);
        }

        auto operator()(tst::type::Array const& array) {
            return format("[{}; {}]", array.element_type, array.length);
        }

        auto operator()(tst::type::Slice const& slice) {
            return format("[{}]", slice.element_type);
        }

        auto operator()(tst::type::Reference const& reference) {
            return format("&{}{}", reference.pointer.mut ? "mut " : "", reference.pointer.type);
        }

        auto operator()(tst::type::Pointer const& pointer) {
            return format("*{}{}", pointer.mut ? "mut " : "", pointer.type);
        }

        auto operator()(tst::type::User_defined_struct const& ud) {
            return format("{}", ud.structure->name);
        }

        auto operator()(tst::type::User_defined_enum const& ud) {
            return format("{}", ud.enumeration->name);
        }

        auto operator()(tst::type::Type_variable const& variable) {
            return format("${}", variable.tag);
        }
    };

}


DEFINE_FORMATTER_FOR(tst::Expression) {
    return std::visit(Expression_format_visitor { { context.out() } }, value.value);
}

DEFINE_FORMATTER_FOR(tst::Type) {
    return std::visit(Type_format_visitor { { context.out() } }, value.value);
}