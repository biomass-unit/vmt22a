#pragma once

#include "bu/utilities.hpp"


namespace bu {

    enum class Flatmap_strategy { store_keys, hash_keys };


    template <
        class Key,
        class Value,
        Flatmap_strategy = std::is_trivially_copyable_v<Key>
            ? Flatmap_strategy::store_keys
            : Flatmap_strategy::hash_keys,
        template <class...> class Container = std::vector
    >
    class Flatmap;


    template <class K, class V, template <class...> class Container>
    class [[nodiscard]] Flatmap<K, V, Flatmap_strategy::store_keys, Container> {
        Container<Pair<K, V>> pairs;
    public:
        using Pairs = Container<Pair<K, V>>;

        using Key   = K;
        using Value = V;

        constexpr Flatmap(decltype(pairs)&& pairs) noexcept
            : pairs { std::move(pairs) } {}

        constexpr Flatmap() noexcept = default;

        constexpr auto add(K&& k, V&& v) &
            noexcept(std::is_nothrow_move_constructible_v<Pair<K, V>>) -> V*
        {
            return std::addressof(pairs.emplace_back(std::move(k), std::move(v)).second);
        }

        constexpr auto find(std::equality_comparable_with<K> auto const& key) const&
            noexcept(noexcept(key == std::declval<K const&>())) -> V const*
        {
            for (auto& [k, v] : pairs) {
                if (key == k) {
                    return std::addressof(v);
                }
            }
            return nullptr;
        }
        constexpr auto find(std::equality_comparable_with<K> auto const& key) &
            noexcept(noexcept(key == std::declval<K const&>())) -> V*
        {
            return const_cast<V*>(const_cast<Flatmap const*>(this)->find(key));
        }

        constexpr auto span() const noexcept -> std::span<Pair<K, V> const> {
            return pairs;
        }

        constexpr auto size() const noexcept -> Usize {
            return pairs.size();
        }

        constexpr decltype(auto) container(this auto&& self) {
            return bu::forward_like<decltype(self)>(self.pairs);
        }

        auto hash() const
            noexcept(noexcept(::bu::hash(pairs))) -> Usize
            requires hashable<K> && hashable<V>
        {
            return hash_combine_with_seed(get_unique_seed(), pairs);
        }

        constexpr auto operator==(Flatmap const&) const noexcept -> bool = default;
    };


    template <class K, class V, template <class...> class Container>
    class [[nodiscard]] Flatmap<K, V, Flatmap_strategy::hash_keys, Container> :
        Flatmap<Usize, V, Flatmap_strategy::store_keys, Container>
    {
        using Parent = Flatmap<Usize, V, Flatmap_strategy::store_keys, Container>;
    public:
        using Parent::Parent;
        using Parent::operator=;
        using Parent::operator==;
        using Parent::span;
        using Parent::size;
        using Parent::container;
        using Parent::hash;

        // The member functions aren't constexpr because the standard library
        // unfortunately does not provide constexpr hashing support.

        auto add(K const& k, V&& v) &
            noexcept(std::is_nothrow_move_constructible_v<V> && noexcept(::bu::hash(k))) -> V*
        {
            return Parent::add(::bu::hash(k), std::move(v));
        }

        auto find(K const& k) const&
            noexcept(noexcept(::bu::hash(k))) -> V const*
        {
            return Parent::find(::bu::hash(k));
        }
        auto find(K const& k) &
            noexcept(noexcept(::bu::hash(k))) -> V*
        {
            return Parent::find(::bu::hash(k));
        }
    };

}