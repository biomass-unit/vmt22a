#pragma once

#include "utilities.hpp"


#define DECLARE_FORMATTER_FOR_TEMPLATE(...)                             \
struct std::formatter<__VA_ARGS__> : std::formatter<std::string_view> { \
    [[nodiscard]] auto format(__VA_ARGS__ const&, std::format_context&) \
        -> std::format_context::iterator;                               \
}

#define DECLARE_FORMATTER_FOR(...) \
template <> DECLARE_FORMATTER_FOR_TEMPLATE(__VA_ARGS__)

#define DEFINE_FORMATTER_FOR(...) \
auto std::formatter<__VA_ARGS__>::format(__VA_ARGS__ const& value, std::format_context& context) \
    -> std::format_context::iterator


template <class... Ts>
DECLARE_FORMATTER_FOR_TEMPLATE(std::variant<Ts...>);
template <class T>
DECLARE_FORMATTER_FOR_TEMPLATE(std::optional<T>);
template <class T>
DECLARE_FORMATTER_FOR_TEMPLATE(std::vector<T>);
template <class F, class S>
DECLARE_FORMATTER_FOR_TEMPLATE(bu::Pair<F, S>);


template <class... Ts>
DEFINE_FORMATTER_FOR(std::variant<Ts...>) {
    return std::visit([out = context.out()](auto& alternative) {
        return std::format_to(out, "{}", alternative);
    }, value);
}

template <class T>
DEFINE_FORMATTER_FOR(std::optional<T>) {
    if (value) {
        return std::format_to(context.out(), "{}", *value);
    }
    else {
        return std::format_to(context.out(), "std::nullopt");
    }
}

template <class T>
DEFINE_FORMATTER_FOR(std::vector<T>) {
    auto out = context.out();

    if (value.empty()) {
        return std::format_to(out, "[]");
    }
    else {
        std::format_to(out, "[{}", value.front());
        for (auto& x : value | std::views::drop(1)) {
            std::format_to(out, ", {}", x);
        }
        return std::format_to(out, "]");
    }
}

template <class F, class S>
DEFINE_FORMATTER_FOR(bu::Pair<F, S>) {
    return std::format_to(context.out(), "({}, {})", value.first, value.second);
}