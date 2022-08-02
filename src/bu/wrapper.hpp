#pragma once

#include "bu/utilities.hpp"


namespace bu {

    template <class...>
    class [[nodiscard]] Wrapper_context;


    template <class T>
    class [[nodiscard]] Wrapper {
        static_assert(std::is_object_v<T>);
        static_assert(!std::is_const_v<T>);

        friend class Wrapper_context<T>;

        inline static Wrapper_context<T>* context_ptr = nullptr;

        static auto context(std::source_location const caller = std::source_location::current())
            noexcept -> Wrapper_context<T>&
        {
            if constexpr (compiling_in_debug_mode) {
                if (!context_ptr) {
                    abort(
                        "bu::Wrapper<{}>: the arena has not been "
                        "initialized"_format(typeid(T).name()),
                        caller
                    );
                }
            }
            return *context_ptr;
        }

        Usize index;
    public:
        template <class... Args>
            requires ((sizeof...(Args) != 1) || (!similar_to<Wrapper, Args> && ...))
        Wrapper(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>)
        {
            this->index = context().make_wrapper(std::forward<Args>(args)...);
        }

        auto operator*() const noexcept -> T const& { return context().get_element(index); }
        auto operator*()       noexcept -> T      & { return context().get_element(index); }

        auto operator->() const noexcept -> T const* { return std::addressof(**this); }
        auto operator->()       noexcept -> T      * { return std::addressof(**this); }

        operator T const&() const& noexcept { return **this; }
        operator T      &()      & noexcept { return **this; }

        operator T const&() const&& = delete;
        operator T      &()      && = delete;

        auto clone(this Wrapper const self)
            noexcept(noexcept(std::is_nothrow_copy_constructible_v<T>)) -> Wrapper
        {
            return Wrapper { *self };
        }

        auto hash(this Wrapper const self)
            noexcept(noexcept(::bu::hash(*self))) -> Usize
            requires hashable<T>
        {
            return ::bu::hash(*self);
        }

        auto unsafe_kill(this Wrapper const self) noexcept -> T&& {
            return context().kill_wrapper(self.index);
        }
    };


    constexpr Usize default_wrapper_arena_capacity = 1024;

    inline constinit bool do_empty_wrapper_arena_debug_messages = true;


    template <class T>
    class Wrapper_context<T> {
        std::vector<T>     arena;
        std::vector<Usize> free_indices;
        bool               is_responsible = true;
    public:
        Wrapper_context(
            Usize capacity = default_wrapper_arena_capacity,
            std::source_location = std::source_location::current());

        Wrapper_context(Wrapper_context&&) noexcept;

        Wrapper_context(Wrapper_context const&) = delete;

        auto operator=(Wrapper_context const &) = delete;
        auto operator=(Wrapper_context      &&) = delete;

        ~Wrapper_context();

        auto get_element(Usize) noexcept -> T&;

        template <class... Args> [[nodiscard]]
        auto make_wrapper(Args&&...) noexcept -> Usize;

        [[nodiscard]]
        auto kill_wrapper(Usize) noexcept -> T&&;

        [[nodiscard]]
        auto arena_size() const noexcept -> Usize;
    };

    template <class T>
    Wrapper_context<T>::Wrapper_context(Usize const capacity, std::source_location const caller) {
        if (Wrapper<T>::context_ptr) {
            bu::abort("bu::Wrapper: Attempted to reinitialize the arena", caller);
        }
        else {
            arena.reserve(capacity);
            Wrapper<T>::context_ptr = this;
        }
    }

    template <class T>
    Wrapper_context<T>::Wrapper_context(Wrapper_context&& other) noexcept
        : arena        { std::move(other.arena)        }
        , free_indices { std::move(other.free_indices) }
    {
        other.is_responsible = false;
        Wrapper<T>::context_ptr = this;
    }

    template <class T>
    Wrapper_context<T>::~Wrapper_context() {
        if (is_responsible) {
            if constexpr (compiling_in_debug_mode) {
                if (do_empty_wrapper_arena_debug_messages && arena.empty()) {
                    bu::print("NOTE: bu::Wrapper<{}>: deallocating empty arena\n", typeid(T).name());
                }
            }

            Wrapper<T>::context_ptr = nullptr;
        }
    }

    template <class T>
    auto Wrapper_context<T>::get_element(Usize const index) noexcept -> T& {
        assert(is_responsible);
        return arena[index];
    }

    template <class T>
    template <class... Args>
    auto Wrapper_context<T>::make_wrapper(Args&&... args) noexcept -> Usize {
        // measure performance

        static_assert(std::is_nothrow_move_assignable_v<T>);
        assert(is_responsible);

        if (free_indices.empty()) {
            Usize const index = arena.size();
            arena.emplace_back(std::forward<Args>(args)...);
            return index;
        }
        else {
            Usize const index = free_indices.back();
            free_indices.pop_back();
            arena[index] = T(std::forward<Args>(args)...);
            return index;
        }
    }

    template <class T>
    auto Wrapper_context<T>::kill_wrapper(Usize const index) noexcept -> T&& {
        assert(is_responsible);
        free_indices.push_back(index);
        return std::move(arena[index]);
    }

    template <class T>
    auto Wrapper_context<T>::arena_size() const noexcept -> bu::Usize {
        return arena.size();
    }


    template <class... Ts>
    class Wrapper_context : Wrapper_context<Ts>... {
    public:
        Wrapper_context(
            Usize                const capacity = default_wrapper_arena_capacity,
            std::source_location const caller = std::source_location::current())
            : Wrapper_context<Ts> { capacity, caller }... {}

        Wrapper_context(Wrapper_context<Ts>&&... children)
            : Wrapper_context<Ts> { std::move(children) }... {}

        template <bu::one_of<Ts...> T> [[nodiscard]]
        auto arena_size() const noexcept -> bu::Usize {
            return Wrapper_context<T>::arena_size();
        }
    };


    template <class T>
    Wrapper(T) -> Wrapper<T>;


    template <class T>
    auto operator==(Wrapper<T> const a, Wrapper<T> const b)
        noexcept(noexcept(*a == *b)) -> bool
    {
        return *a == *b;
    }

    template <class A, class B>
    auto operator==(Wrapper<A> const a, B const& b)
        noexcept(noexcept(*a == b)) -> bool
    {
        return *a == b;
    }

    template <class A, class B>
    auto operator==(A const& a, Wrapper<B> const b)
        noexcept(noexcept(a == *b)) -> bool
    {
        return a == *b;
    }


    constexpr auto wrap = []<class X>(X&& x) {
        return Wrapper<std::decay_t<X>> { std::forward<X>(x) };
    };


    template <class T>
    concept wrapper = instance_of<T, Wrapper>;

}


template <class T>
struct std::formatter<bu::Wrapper<T>> : std::formatter<T> {
    auto format(bu::Wrapper<T> const wrapper, std::format_context& context) {
        return std::formatter<T>::format(*wrapper, context);
    }
};