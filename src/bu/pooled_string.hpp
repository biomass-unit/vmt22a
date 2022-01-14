#pragma once

#include "bu/utilities.hpp"


namespace bu {

    template <class /* tag */>
    class [[nodiscard]] Pooled_string {
        bu::Usize index;

        inline static auto vector =
            bu::vector_with_capacity<Pair<std::string, bu::Usize>>(256);

        explicit Pooled_string(auto&& string, auto f) noexcept {
            auto const hash = std::hash<std::string_view>{}(string);
            auto const it = std::ranges::find(vector, hash, bu::second);

            if (it != vector.end()) {
                index = bu::unsigned_distance(vector.begin(), it);
            }
            else {
                index = vector.size();
                vector.emplace_back(f(string), hash);
            }
        }
    public:
        explicit Pooled_string(std::string&& string) noexcept
            : Pooled_string(std::move(string), bu::move) {}
        explicit Pooled_string(std::string_view string) noexcept
            : Pooled_string(string, bu::make<std::string>) {}

        [[nodiscard]]
        auto view() const noexcept -> std::string_view {
            return vector[index].first;
        }

        [[nodiscard]]
        auto operator==(Pooled_string const& other) const noexcept -> bool {
            return index == other.index;
        }

        static auto release() noexcept -> void {
            decltype(vector){}.swap(vector);
        }
    };

}


template <class Tag>
struct std::formatter<bu::Pooled_string<Tag>> : std::formatter<std::string_view> {
    auto format(bu::Pooled_string<Tag> const string, std::format_context& context) {
        return std::formatter<std::string_view>::format(string.view(), context);
    }
};