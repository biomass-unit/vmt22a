#pragma once

#include "bu/utilities.hpp"


namespace bu {

    enum class Pooled_string_strategy {
        guaranteed_new_string,
        potential_new_string
    };

    template <class /* tag */>
    class [[nodiscard]] Pooled_string {
    public:
        friend struct std::hash<Pooled_string>;
    private:
        Usize index;

        inline static auto vector =
            vector_with_capacity<Pair<std::string, bu::Usize>>(256);

        explicit Pooled_string(auto&& string, auto f, Pooled_string_strategy const strategy) noexcept {
            Usize const hash = std::hash<std::string_view>{}(string);

            if (strategy == Pooled_string_strategy::potential_new_string) {
                auto const it = std::ranges::find(vector, hash, bu::second);
                if (it != vector.end()) {
                    index = unsigned_distance(vector.begin(), it);
                    return;
                }
            }

            index = vector.size();
            vector.emplace_back(f(string), hash);
        }
    public:
        explicit Pooled_string(
            std::string&&                string,
            Pooled_string_strategy const strategy = Pooled_string_strategy::potential_new_string
        ) noexcept
            : Pooled_string(std::move(string), bu::move, strategy) {}

        explicit Pooled_string(
            std::string_view       const string,
            Pooled_string_strategy const strategy = Pooled_string_strategy::potential_new_string
        ) noexcept
            : Pooled_string(string, bu::make<std::string>, strategy) {}

        [[nodiscard]]
        auto view() const noexcept -> std::string_view {
            return vector[index].first;
        }
        [[nodiscard]]
        auto hash() const noexcept -> bu::Usize {
            return vector[index].second;
        }

        [[nodiscard]]
        auto operator==(Pooled_string const& other) const noexcept -> bool {
            return index == other.index; // could be defaulted but this explicitly shows how cheap the operation is
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