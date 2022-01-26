#pragma once

#include "bu/utilities.hpp"


namespace bu {

    template <class /* tag */>
    class [[nodiscard]] Pooled_string {
    public:
        enum Strategy {
            guaranteed_new_string,
            potential_new_string,
        };
    private:
        bu::Usize index;

        inline static auto vector =
            bu::vector_with_capacity<Pair<std::string, bu::Usize>>(256);

        explicit Pooled_string(auto&& string, auto f, Strategy strategy) noexcept {
            auto const hash = std::hash<std::string_view>{}(string);

            if (strategy == potential_new_string) {
                auto const it = std::ranges::find(vector, hash, bu::second);
                if (it != vector.end()) {
                    index = bu::unsigned_distance(vector.begin(), it);
                    return;
                }
            }

            index = vector.size();
            vector.emplace_back(f(string), hash);
        }
    public:
        explicit Pooled_string(std::string&& string, Strategy strategy = potential_new_string) noexcept
            : Pooled_string(std::move(string), bu::move, strategy) {}
        explicit Pooled_string(std::string_view string, Strategy strategy = potential_new_string) noexcept
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
            decltype(vector){}.swap(vector);
        }
        static auto clear() noexcept -> void {
            vector.clear();
        }
    };

}


template <class Tag>
struct std::formatter<bu::Pooled_string<Tag>> : bu::Formatter_base {
    auto format(bu::Pooled_string<Tag> const string, std::format_context& context) {
        return std::format_to(context.out(), "{}", string.view());
    }
};