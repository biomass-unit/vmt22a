#pragma once

#include "bu/utilities.hpp"


namespace bu {

    class [[nodiscard]] Bytestack {
        std::unique_ptr<std::byte[]> buffer;
        std::byte* pointer;
    public:
        explicit Bytestack(Usize capacity) noexcept
            : buffer { std::make_unique_for_overwrite<std::byte[]>(capacity) }
            , pointer { buffer.get() } {}

        template <trivial T>
        auto push(T const x) noexcept -> void {
            std::memcpy(pointer, &x, sizeof x);
            pointer += sizeof x;
        }

        template <trivial T>
        auto pop() noexcept -> T {
            T x;
            pointer -= sizeof x;
            std::memcpy(&x, pointer, sizeof x);
            return x;
        }

        template <trivial T>
        auto top() const noexcept -> T {
            T x;
            std::memcpy(&x, pointer - sizeof x, sizeof x);
            return x;
        }
    };

}