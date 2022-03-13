#pragma once

#include "bu/utilities.hpp"
#include "bu/bytestack.hpp"
#include "bytecode.hpp"


namespace vm {

    using Jump_offset_type  = bu::Usize;
    using Local_offset_type = bu::I16; // signed because function parameters use negative indexing
    using Local_size_type   = std::make_unsigned_t<Local_offset_type>;


    struct Activation_record {
        std::byte*         return_value_address;
        std::byte*         return_address;
        Activation_record* caller;

        auto pointer() noexcept -> std::byte* {
            return reinterpret_cast<std::byte*>(this);
        }
    };


    struct [[nodiscard]] Virtual_machine {
        Bytecode           bytecode;
        bu::Bytestack      stack;
        std::byte*         instruction_pointer = nullptr;
        std::byte*         instruction_anchor  = nullptr;
        Activation_record* activation_record   = nullptr;
        bool               keep_running        = true;

        static constexpr bu::Usize version = 0;


        auto run() -> int;

        auto jump_to(Jump_offset_type) noexcept -> void;

        template <bu::trivial T>
        auto extract_argument() noexcept -> T;


        std::string output_buffer;

        auto flush_output() -> void;


        struct String {
            char const* pointer;
            bu::Usize   length;
        };

        std::string                      string_buffer;
        std::vector<bu::Pair<bu::Usize>> string_buffer_views;
        std::vector<String>              string_pool;

        auto add_to_string_pool(std::string_view const string) noexcept -> bu::Usize {
            auto const offset = string_buffer.size();
            string_buffer.append(string);
            string_buffer_views.emplace_back(offset, string.size());
            return string_buffer_views.size() - 1;
        }
    };

}