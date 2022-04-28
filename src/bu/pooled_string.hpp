#pragma once

#include "bu/utilities.hpp"


namespace bu {

    enum class Pooled_string_strategy {
        guaranteed_new_string,
        potential_new_string
    };

    template <class /* tag */>
    class [[nodiscard]] Pooled_string {
        Usize index;

        inline static auto vector =
            vector_with_capacity<Pair<std::string, bu::Usize>>(256);

        explicit Pooled_string(auto&& string, auto f, Pooled_string_strategy const strategy)
            noexcept(bu::compiling_in_release_mode)
        {
            Usize const hash = std::hash<std::string_view>{}(string);

            if (strategy == Pooled_string_strategy::potential_new_string) {
                auto const it = std::ranges::find(vector, hash, bu::second);
                if (it != vector.end()) {
                    index = unsigned_distance(vector.begin(), it);
                    return;
                }
            }
            else if constexpr (bu::compiling_in_debug_mode) {
                auto const it = std::ranges::find(vector, hash, bu::second);
                if (it != vector.end()) {
                    bu::abort(std::format("A string with the value '{}' already exists", it->first));
                }
            }

            index = vector.size();
            vector.emplace_back(f(string), hash);
        }
    public:
        explicit Pooled_string(
            std::string&&                string,
            Pooled_string_strategy const strategy = Pooled_string_strategy::potential_new_string
        ) noexcept(bu::compiling_in_release_mode)
            : Pooled_string(std::move(string), bu::move, strategy) {}

        explicit Pooled_string(
            std::string_view       const string,
            Pooled_string_strategy const strategy = Pooled_string_strategy::potential_new_string
        ) noexcept(bu::compiling_in_release_mode)
            : Pooled_string(string, bu::make<std::string>, strategy) {}

        [[nodiscard]]
        auto view(this Pooled_string const self) noexcept -> std::string_view {
            return vector[self.index].first;
        }

        [[nodiscard]]
        auto hash(this Pooled_string const self) noexcept -> Usize {
            return vector[self.index].second;
        }

        [[nodiscard]]
        auto operator==(this Pooled_string const self, Pooled_string const other) noexcept -> bool {
            // Could be defaulted but this explicitly shows how cheap the operation is
            return self.index == other.index;
        }

        static auto release() noexcept -> void {
            decltype(vector) {}.swap(vector);
        }
    };

}


template <bu::instance_of<bu::Pooled_string> String>
struct std::formatter<String> : std::formatter<std::string_view> {
    auto format(String const string, std::format_context& context) {
        return std::formatter<std::string_view>::format(string.view(), context);
    }
};