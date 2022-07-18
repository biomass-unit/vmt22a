#include "bu/utilities.hpp"
#include "inference.hpp"


auto inference::unit_type() noexcept -> Type {
    return { type::Tuple {} };
}

auto inference::Context::fresh_type_variable(type::Variable::Kind const kind) -> Type {
    return { type::Variable { .tag { .value = current_type_variable_tag++.get() }, .kind = kind } };
}


namespace {

    struct Type_formatting_visitor : bu::fmt::Visitor_base {
        auto operator()(inference::type::Boolean) {
            return format("Bool");
        }
        auto operator()(inference::type::Integer const integer) {
            static constexpr auto strings = std::to_array<std::string_view>({
                "i8", "i16", "i32", "i64",
                "u8", "u16", "u32", "u64",
            });
            static_assert(strings.size() == static_cast<bu::Usize>(inference::type::Integer::_integer_count));
            return format(strings[static_cast<bu::Usize>(integer)]);
        }
        auto operator()(inference::type::Floating const floating) {
            switch (floating) {
            case inference::type::Floating::f32:
                return format("f32");
            case inference::type::Floating::f64:
                return format("f64");
            default:
                std::unreachable();
            }
        }
        auto operator()(inference::type::Tuple const& tuple) {
            return format("({})", tuple.types);
        }
        auto operator()(inference::type::Function const& function) {
            return format("fn({}): {}", function.arguments, function.return_type);
        }
        auto operator()(inference::type::Reference const& reference) {
            return format("&{}{}", reference.mutability, reference.referenced_type);
        }
        auto operator()(inference::type::Variable const& variable) {
            return format(
                "'{}{}",
                [k = variable.kind] {
                    using enum inference::type::Variable::Kind;
                    switch (k) {
                    case any_integer:    return 'I';
                    case signed_integer: return 'S';
                    case floating:       return 'F';
                    case general:        return 'T';
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
    return std::visit(Type_formatting_visitor { { context.out() } }, value);
}