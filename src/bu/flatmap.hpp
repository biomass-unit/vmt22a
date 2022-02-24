#pragma once

#include "bu/utilities.hpp"


namespace bu {

    enum class Flatmap_strategy { store_keys, hash_keys };


    template <class, class, Flatmap_strategy = Flatmap_strategy::hash_keys>
    class Flatmap;

    template <class K, class V>
    class [[nodiscard]] Flatmap<K, V, Flatmap_strategy::store_keys> {
        std::vector<Pair<K, V>> pairs;
    public:
        explicit constexpr Flatmap(Usize const capacity = 32) noexcept
            : pairs { bu::vector_with_capacity<Pair<K, V>>(capacity) } {}

        constexpr auto add(K&& k, V&& v)
            noexcept(std::is_nothrow_move_constructible_v<Pair<K, V>>) -> void
        {
            pairs.emplace_back(std::move(k), std::move(v));
        }

        constexpr auto find(std::equality_comparable_with<K> auto const& key) const
            noexcept(noexcept(key == std::declval<K const&>())) -> V const*
        {
            for (auto& [k, v] : pairs) {
                if (key == k) {
                    return std::addressof(v);
                }
            }
            return nullptr;
        }
        constexpr auto find(std::equality_comparable_with<K> auto const& key)
            noexcept(noexcept(key == std::declval<K const&>())) -> V*
        {
            return const_cast<V*>(const_cast<Flatmap const*>(this)->find(key));
        }

        constexpr auto size() const noexcept -> Usize {
            return pairs.size();
        }
    };

    template <class K, class V>
    class [[nodiscard]] Flatmap<K, V, Flatmap_strategy::hash_keys> :
        Flatmap<Usize, V, Flatmap_strategy::store_keys>
    {
        using Parent = Flatmap<Usize, V, Flatmap_strategy::store_keys>;

        static constexpr std::hash<K> hash;
    public:
        using Parent::Parent;
        using Parent::operator=;
        using Parent::size;

        // The member functions aren't constexpr because
        // std::hash::operator() unfortunately isn't constexpr.

        auto add(K const& k, V&& v)
            noexcept(std::is_nothrow_move_constructible_v<V> && noexcept(hash(k))) -> void
        {
            Parent::add(hash(k), std::move(v));
        }

        auto find(K const& k) const
            noexcept(noexcept(hash(k))) -> V const*
        {
            return Parent::find(hash(k));
        }
        auto find(K const& k)
            noexcept(noexcept(hash(k))) -> V*
        {
            return Parent::find(hash(k));
        }
    };

}