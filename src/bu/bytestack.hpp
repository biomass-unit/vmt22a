#pragma once

#include "bu/utilities.hpp"


namespace bu {

    class [[nodiscard]] Bytestack {

        std::unique_ptr<std::byte[]> buffer;
        Usize                        length;

    public:
        std::byte* pointer;
    private:
        std::byte* top_pointer;
    public:

        explicit Bytestack(Usize const capacity) noexcept
            : buffer { std::make_unique_for_overwrite<std::byte[]>(capacity) }
            , length { capacity }
            , pointer { buffer.get() }
            , top_pointer { pointer + capacity } {}

        template <trivial T>
        auto push(T const x) noexcept -> void {
            if (pointer + sizeof x > top_pointer) [[unlikely]] {
                bu::abort("stack overflow");
            }

            std::memcpy(pointer, &x, sizeof x);
            pointer += sizeof x;
        }

        template <trivial T>
        auto pop() noexcept -> T {
            if (buffer.get() + sizeof(T) > pointer) [[unlikely]] {
                bu::abort("stack underflow");
            }

            T x;
            pointer -= sizeof x;
            std::memcpy(&x, pointer, sizeof x);
            return x;
        }

        template <trivial T>
        auto top() const noexcept -> T {
            if (buffer.get() + sizeof(T) > pointer) [[unlikely]] {
                bu::abort("stack underflow");
            }

            T x;
            std::memcpy(&x, pointer - sizeof x, sizeof x);
            return x;
        }

        auto base()       noexcept -> std::byte      * { return buffer.get(); }
        auto base() const noexcept -> std::byte const* { return buffer.get(); }

        auto capacity() const noexcept -> Usize { return length; }

    };

}