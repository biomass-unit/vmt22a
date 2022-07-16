#include "bu/utilities.hpp"
#include "inference.hpp"


auto inference::unit_type() noexcept -> Type::Variant {
    return type::Tuple {};
}

auto inference::Context::fresh_type_variable(type::Variable::Kind const kind) -> type::Variable {
    return { .tag { .value = current_type_variable_tag++.get() }, .kind = kind };
}


namespace {

    struct Type_formatting_visitor : bu::fmt::Visitor_base {
        auto operator()(inference::type::Integer const integer) {
            static constexpr auto strings = std::to_array<std::string_view>({
                "i8", "i16", "i32", "i64",
                "u8", "u16", "u32", "u64",
            });
            static_assert(strings.size() == static_cast<bu::Usize>(inference::type::Integer::_integer_count));
            return format(strings[static_cast<bu::Usize>(integer)]);
        }
        auto operator()(inference::type::Floating const floating) {
            using enum inference::type::Floating;
            return format(
                floating == f32
                    ? "f32"
                    : floating == f64
                        ? "f64"
                        : bu::hole()
            );
        }
        auto operator()(inference::type::Tuple const& tuple) {
            return format("({})", tuple.types);
        }
        auto operator()(inference::type::Variable const& variable) {
            return format(
                "'{}{}",
                [k = variable.kind] {
                    using enum inference::type::Variable::Kind;
                    switch (k) {
                    case integer:  return 'I';
                    case floating: return 'F';
                    case general:  return 'T';
                    default:
                        std::unreachable();
                    }
                }(),
                variable.tag.value
            );
        }
    };

}


DEFINE_FORMATTER_FOR(inference::Type) {
    return std::visit(Type_formatting_visitor { { context.out() } }, value.value);
}