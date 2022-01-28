#pragma once

#include "bu/utilities.hpp"


namespace vm {

    struct Bytecode {
        std::vector<std::byte> bytes;

        auto write(bu::trivial auto const... args) noexcept -> void {
            bu::serialize_to(std::back_inserter(bytes), args...);
        }

        auto current_offset() const noexcept -> bu::Usize {
            return bytes.size();
        }
    };

}