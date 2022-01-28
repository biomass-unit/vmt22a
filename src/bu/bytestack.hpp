#pragma once

#include "bu/utilities.hpp"


namespace bu {

    // Stack overflow & underflow detection only in debug mode

    class [[nodiscard]] Bytestack {
        std::unique_ptr<std::byte[]> buffer;
        std::byte* pointer;

#ifndef NDEBUG
        std::byte* top_pointer;
#endif
    public:
        explicit Bytestack(Usize const capacity) noexcept
            : buffer { std::make_unique_for_overwrite<std::byte[]>(capacity) }
            , pointer { buffer.get() }
#ifndef NDEBUG
            , top_pointer { pointer + capacity }
#endif
        {}

        template <trivial T>
        auto push(T const x) noexcept -> void {
            assert("stack overflow" && pointer + sizeof x <= top_pointer);

            std::memcpy(pointer, &x, sizeof x);
            pointer += sizeof x;
        }

        template <trivial T>
        auto pop() noexcept -> T {
            assert("stack underflow" && buffer.get() + sizeof(T) <= pointer);

            T x;
            pointer -= sizeof x;
            std::memcpy(&x, pointer, sizeof x);
            return x;
        }

        template <trivial T>
        auto top() const noexcept -> T {
            assert("stack underflow" && buffer.get() + sizeof(T) <= pointer);

            T x;
            std::memcpy(&x, pointer - sizeof x, sizeof x);
            return x;
        }
    };

}